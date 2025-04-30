/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 * inotify enables monitoring of filesystem activity
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "intern.h"
#include "core.h"
#include "dict.h"
#include "inotify.h"
#include "threads.h"
#include "epoll.h"

#ifndef CRTX_TEST

// we use one fd for all inotify listeners
static int inotify_fd = -1;

static struct crtx_inotify_listener **listeners = 0;
static unsigned int n_listeners = 0;
MUTEX_TYPE mutex;

static struct crtx_thread_job_description thread_job = {0};
static struct crtx_thread *global_thread = 0;
static struct crtx_evloop_fd evloop_fd = {};
static struct crtx_evloop_callback el_cb = {};
static char stop = 0;

#define INOFITY_BUFLEN (sizeof(struct inotify_event) + NAME_MAX + 1)

#define EV(name) { IN_ ## name, #name, sizeof(#name) }

struct crtx_inotify_event_dict {
	uint32_t type;
	char *name;
	size_t name_size;
} crtx_inotify_event_dict[] = {
	EV(ACCESS),
	EV(MODIFY),
	EV(ATTRIB),
	EV(CLOSE_WRITE),
	EV(CLOSE_NOWRITE),
	EV(OPEN),
	EV(MOVED_FROM),
	EV(MOVED_TO),
	EV(CREATE),
	EV(DELETE),
	EV(DELETE_SELF),
	
	EV(DONT_FOLLOW),
	EV(EXCL_UNLINK),
	EV(MASK_ADD),
	EV(ONESHOT),
	EV(ONLYDIR),
	EV(IGNORED),
	EV(ISDIR),
	EV(Q_OVERFLOW),
	EV(UNMOUNT),
	{0, 0}
};

uint32_t crtx_inotify_string2mask(char *mask_string) {
	struct crtx_inotify_event_dict *dictit;
	
	if (mask_string) {
		dictit = crtx_inotify_event_dict;
		while (dictit->name) {
			if (!strcmp(dictit->name, mask_string)) {
				return dictit->type;
			}
			dictit++;
		}
	}
	
	CRTX_ERROR("inotify: no mask \"%s\" found\n", mask_string);
	return 0;
}

int crtx_inotify_mask2string(uint32_t mask, char *string, size_t *string_size) {
	struct crtx_inotify_event_dict *dictit;
	char *pos;
	
	if (mask && string_size) {
		pos = string;
		
		if (!string)
			*string_size = 0;
		
		dictit = crtx_inotify_event_dict;
		while (dictit->name) {
			if (dictit->type & mask) {
				if (string) {
					pos += snprintf(pos, *string_size - (pos-string), "%s ", dictit->name);
				} else {
					*string_size += dictit->name_size - 1 + 1; // without \0 but with ","
				}
			}
			dictit++;
		}
		
		// remove last " "
		if (pos > string)
			pos[-1] = 0; // we request 1 byte more than necessary
		
		return 1;
	} else {
		CRTX_ERROR("error while executing crtx_inotify_string2mask\n");
		return 0;
	}
}

int crtx_inotify_to_dict(struct inotify_event *iev, struct crtx_dict **dict) {
	size_t len;
	unsigned char mlen;
	struct crtx_dict *mask_dict;
	struct crtx_inotify_event_dict *dictit;
	struct crtx_dict_item *di;
	
	
	len = iev->len;
	
	mlen = CRTX_POPCOUNT32(iev->mask);
	mask_dict = crtx_init_dict(0, mlen, 0);
	
	dictit = crtx_inotify_event_dict;
	di = mask_dict->items;
	while (dictit->name) {
		if (dictit->type & iev->mask) {
			di->type = 's';
			di->key = 0;
			
			di->string = dictit->name;
			di->size = dictit->name_size;
			di->flags = CRTX_DIF_DONT_FREE_DATA;
			
			di++;
		}
		dictit++;
	}
	di--;
	di->flags |= CRTX_DIF_LAST_ITEM;
	
	*dict = crtx_create_dict("iDuus",
		"wd", iev->wd, sizeof(int32_t), 0,
		"mask", mask_dict, mask_dict->size, 0,
		"cookie", iev->cookie, sizeof(uint32_t), 0,
		"len", iev->len, sizeof(uint32_t), 0,
		"name", len?crtx_stracpy(iev->name, &len):0, len, 0
		);
	
	return 0;
}

static int inotify_process() {
	char buffer[INOFITY_BUFLEN];
	ssize_t length;
	size_t i;
	unsigned int j;
	struct crtx_event *event;
	struct crtx_inotify_listener *fal;
	struct inotify_event *in_event;
	
	
	length = read(inotify_fd, buffer, INOFITY_BUFLEN);
	
	if (stop)
		return 0;
	
	if (length < 0) {
		CRTX_ERROR("inotify read failed\n");
		return length;
	}
	
	i=0;
	while (i < length) {
		in_event = (struct inotify_event *) &buffer[i];
		
		fal = 0;
		LOCK(mutex);
		for (j=0; j < n_listeners; j++) {
			if (listeners[j]->wd == in_event->wd) {
				fal = listeners[j];
				break;
			}
		}
		UNLOCK(mutex);
		
		if (fal) {
			struct inotify_event *ev_data;
			
			ev_data = (struct inotify_event *) malloc(sizeof(struct inotify_event) + in_event->len);
			memcpy(ev_data, in_event, sizeof(struct inotify_event) + in_event->len);
			
			crtx_create_event(&event);
			
			if (fal->base.graph->descriptions)
				event->description = fal->base.graph->descriptions[0];
			
			crtx_event_set_raw_data(event, 'p', ev_data, sizeof(struct inotify_event) + in_event->len, 0);
			
			crtx_add_event(fal->base.graph, event);
		} else {
			CRTX_ERROR("inotify listener for wd %d not found\n", in_event->wd);
		}
		
		i += sizeof(struct inotify_event) + in_event->len;
	}
	
	return 0;
}

void *inotify_tmain(void *data) {
	while (!stop) {
		if (inotify_process())
			break;
	}
	
	return 0;
}

static int inotify_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	inotify_process();
	
	return 0;
}

static char stop_inotify_listener(struct crtx_listener_base *listener) {
	struct crtx_inotify_listener *inlist;
	
	inlist = (struct crtx_inotify_listener*) listener;
	
	inotify_rm_watch(inotify_fd, inlist->wd);
	
	return 0;
}

static void shutdown_inotify_listener(struct crtx_listener_base *listener) {
	unsigned int i, closefd;
	
	closefd = 1;
	for (i=0; i < n_listeners; i++) {
		if (listeners[i] == (struct crtx_inotify_listener*) listener) {
			listeners[i] = 0;
		}
		if (listeners[i])
			closefd = 0;
	}
	
	if (closefd) {
		if (inotify_fd >= 0) {
			close(inotify_fd);
			inotify_fd = -1;
		}
	}
}

static void stop_thread(struct crtx_thread *thread, void *data) {
	stop = 1;
	if (inotify_fd >= 0)
		close(inotify_fd);
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_inotify_listener *inlist;
	
	
	inlist = (struct crtx_inotify_listener*) listener;
	
	// is this the first inotify listener?
	if (inotify_fd == -1) {
		enum crtx_processing_mode mode;
		
		
		inotify_fd = inotify_init();
		if (inotify_fd == -1) {
			printf("inotify initialization failed\n");
			return -1;
		}
		
		INIT_MUTEX(mutex);
		
		// get default mode or use NONE as fallback
		mode = crtx_get_mode(CRTX_PREFER_NONE);
		
		if (mode == CRTX_PREFER_THREAD) {
			thread_job.fct = &inotify_tmain;
			thread_job.do_stop = &stop_thread;
			global_thread = crtx_thread_assign_job(&thread_job);
			
			crtx_reference_signal(&global_thread->finished);
			
			crtx_thread_start_job(global_thread);
		} else
		if (mode == CRTX_PREFER_ELOOP) {
			crtx_evloop_create_fd_entry(&evloop_fd, &el_cb,
				&inotify_fd, 0,
				CRTX_EVLOOP_READ,
				0,
				&inotify_fd_event_handler,
				0,
				0, 0
				);
			
			crtx_evloop_enable_cb(&el_cb);
		}
	}
	
	// only the main listener is added to the main loop and it will trigger the sub-listeners
	inlist->base.mode = CRTX_NO_PROCESSING_MODE;
	
	inlist->wd = inotify_add_watch(inotify_fd, inlist->path, inlist->mask);
	if (inlist->wd == -1) {
		printf("inotify_add_watch(\"%s\", %u) failed with %d: %s\n", inlist->path, inlist->mask, errno, strerror(errno));
		return -errno;
	}
	
	return 0;
}

struct crtx_listener_base *crtx_setup_inotify_listener(void *options) {
	struct crtx_inotify_listener *inlist;
	unsigned int list_idx;
	
	
	inlist = (struct crtx_inotify_listener*) options;
	
	LOCK(mutex);
	if (listeners == 0) {
		n_listeners = 1;
		list_idx = 0;
		listeners = (struct crtx_inotify_listener**) malloc(sizeof(struct crtx_inotify_listener*));
	} else {
		unsigned int i;
		for (i=0; i < n_listeners; i++) {
			if (listeners[i] == 0) {
				list_idx = i;
				break;
			}
		}
		
		if (i == n_listeners) {
			list_idx = n_listeners;
			n_listeners++;
			listeners = (struct crtx_inotify_listener**) realloc(listeners, sizeof(struct crtx_inotify_listener*)*n_listeners);
		}
	}
	UNLOCK(mutex);
	listeners[list_idx] = inlist;
	
	
	inlist->base.start_listener = &start_listener;
	inlist->base.stop_listener = &stop_inotify_listener;
	inlist->base.shutdown = &shutdown_inotify_listener;
	
	return &inlist->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(inotify)

void crtx_inotify_init() {
}

void crtx_inotify_finish() {
	if (listeners) {
		free(listeners);
		listeners = 0;
		if (inotify_fd >= 0) {
			close(inotify_fd);
			inotify_fd = -1;
		}
	}
}

#else /* CRTX_TEST */

struct crtx_inotify_listener lstnr;

static int event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;
	
	crtx_inotify_to_dict((struct inotify_event *) crtx_event_get_ptr(event), &dict);
	
	crtx_print_dict(dict);
	
	crtx_dict_unref(dict);
	
	return 0;
}

int test_main(int argc, char **argv) {
	int ret;
	
	
	memset(&lstnr, 0, sizeof(struct crtx_inotify_listener));
	
	if (argc > 1) {
		lstnr.path = argv[1];
	} else {
		fprintf(stderr, "Usage: %s <path> [mask1,mask2,...]\n", argv[0]);
		return 1;
	}
	
	if (argc > 2) {
		char *save, *s, *sub;
		
		lstnr.mask = 0;
		s = argv[2];
		while (1) {
			sub = strtok_r(s, ",", &save);
			if (sub == NULL)
				break;
			s = 0;
			lstnr.mask |= crtx_inotify_string2mask(sub);
		}
	} else {
		lstnr.mask = IN_CREATE | IN_DELETE;
	}
	
	ret = crtx_setup_listener("inotify", &lstnr);
	if (ret) {
		CRTX_ERROR("create_listener(inotify) failed: %s\n", strerror(-ret));
		return 0;
	}
	
	ret = crtx_start_listener(&lstnr.base);
	if (ret) {
		CRTX_ERROR("starting inotify listener failed\n");
		return 0;
	}
	
	crtx_create_task(lstnr.base.graph, 0, "test_handler", &event_handler, 0);
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(test_main);

#endif /* CRTX_TEST */
