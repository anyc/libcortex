
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "core.h"
#include "dict.h"
#include "inotify.h"
#include "threads.h"

static int inotify_fd = -1;
// char *inotify_msg_etype[] = { INOTIFY_MSG_ETYPE, 0 };

static struct crtx_inotify_listener **listeners = 0;
static unsigned int n_listeners = 0;
MUTEX_TYPE mutex;

struct crtx_thread *t;
static char stop = 0;

#define INOFITY_BUFLEN (sizeof(struct inotify_event) + NAME_MAX + 1)

#define EV(name) { IN_ ## name, #name, sizeof(#name) }

struct inotify_event_dict {
	uint32_t type;
	char *name;
	size_t name_size;
} event_dict[] = {
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
	EV(UNMOUNT),
	EV(Q_OVERFLOW),
	EV(IGNORED),
	
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
	struct inotify_event_dict *dictit;
	
	if (mask_string) {
		dictit = event_dict;
		while (dictit->name) {
			if (!strcmp(dictit->name, mask_string)) {
				return dictit->type;
			}
			dictit++;
		}
	}
	
	ERROR("inotify: no mask \"%s\" found\n", mask_string);
	return 0;
}

char crtx_inotify_mask2string(uint32_t mask, char *string, size_t *string_size) {
	struct inotify_event_dict *dictit;
	char *pos;
	
	if (mask && string_size) {
		pos = string;
		
		if (!string)
			*string_size = 0;
		
		dictit = event_dict;
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
// 		*string_size += 1 - 1; // add final \0, remove last " "
		// remove last " "
		if (pos > string)
			pos[-1] = 0; // we request 1 byte more than necessary
		
		return 1;
	} else {
		ERROR("error while executing crtx_inotify_string2mask\n");
		return 0;
	}
}

void inotify_to_dict(struct crtx_event_data *data) {
	struct inotify_event *iev;
	size_t len;
	unsigned char mlen;
	struct crtx_dict *mask_dict;
	char *mask_signature; //[33]
	struct inotify_event_dict *dictit;
	struct crtx_dict_item *di;
	
	iev = (struct inotify_event*) data->raw.pointer;
	len = strlen(iev->name);
	
	mlen = POPCOUNT32(iev->mask);
	mask_signature = (char*) malloc(mlen + 1);
	memset(mask_signature, 's', mlen);
	mask_signature[mlen] = 0;
	
	mask_dict = crtx_init_dict(mask_signature, mlen, 0);
	
	dictit = event_dict;
	di = mask_dict->items;
	while (dictit->name) {
		if (dictit->type & iev->mask) {
			di->type = 's';
			di->key = 0;
			
			di->string = dictit->name;
			di->size = dictit->name_size;
			di->flags = DIF_DATA_UNALLOCATED;
			
			di++;
		}
		dictit++;
	}
	di--;
	di->flags |= DIF_LAST;
	
	data->dict = crtx_create_dict("iDuus",
					"wd", iev->wd, sizeof(int32_t), 0,
					"mask", mask_dict, mask_dict->size, 0,
					"cookie", iev->cookie, sizeof(uint32_t), 0,
					"len", iev->len, sizeof(uint32_t), 0,
					"name", crtx_stracpy(iev->name, &len), len, 0
				);
}

void *inotify_tmain(void *data) {
	char buffer[INOFITY_BUFLEN];
	ssize_t length;
	size_t i;
	unsigned int j;
	
	struct crtx_event *event;
	struct crtx_inotify_listener *fal;
	struct inotify_event *in_event;
	
// 	fal = (struct crtx_inotify_listener*) data;
	
	while (!stop) {
		length = read(inotify_fd, buffer, INOFITY_BUFLEN);
		
		if (stop)
			break;
		
		if (length < 0) {
			printf("inotify read failed\n");
			break;
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
				
				event = create_event(fal->parent.graph->types[0], ev_data, sizeof(struct inotify_event) + in_event->len);
				event->data.raw_to_dict = &inotify_to_dict;
				
// 				reference_event_release(event);
				
				add_event(fal->parent.graph, event);
				
// 				wait_on_event(event);
				
// 				event->data.raw = 0;
				
// 				dereference_event_release(event);
			} else {
				ERROR("inotify listener for wd %d not found\n", in_event->wd);
			}
			
			i += sizeof(struct inotify_event) + in_event->len;
		}
	}
	
	return 0;
}

void crtx_shutdown_inotify_listener(struct crtx_listener_base *data) {
	unsigned int i;
	
	for (i=0; i < n_listeners; i++) {
		if (listeners[i] == (struct crtx_inotify_listener*) data) {
			inotify_rm_watch(inotify_fd, listeners[i]->wd);
			listeners[i] = 0;
		}
	}
}

static void stop_thread(struct crtx_thread *thread, void *data) {
	stop = 1;
	close(inotify_fd);
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_inotify_listener *inlist;
	
	inlist = (struct crtx_inotify_listener*) listener;
	
	inlist->wd = inotify_add_watch(inotify_fd, inlist->path, inlist->mask);
	if (inlist->wd == -1) {
		printf("inotify_add_watch(\"%s\") failed with %d: %s\n", inlist->path, errno, strerror(errno));
		return 0;
	}
	
	return 1;
}

struct crtx_listener_base *crtx_new_inotify_listener(void *options) {
	struct crtx_inotify_listener *inlist;
	unsigned int list_idx;
	
	inlist = (struct crtx_inotify_listener*) options;
	
	if (inotify_fd == -1) {
		inotify_fd = inotify_init();
		if (inotify_fd == -1) {
			printf("inotify initialization failed\n");
			return 0;
		}
		
		INIT_MUTEX(mutex);
		
		t = get_thread(inotify_tmain, 0, 1);
		t->do_stop = &stop_thread;
	}
	
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
	
	
	inlist->parent.shutdown = &crtx_shutdown_inotify_listener;
	
// 	new_eventgraph(&inlist->parent.graph, 0, inotify_msg_etype);
	
	inlist->parent.thread = 0;
	inlist->parent.start_listener = &start_listener;
	
	return &inlist->parent;
}

void crtx_inotify_init() {
}

void crtx_inotify_finish() {
	unsigned int i;
	
	if (listeners) {
		for (i=0; i < n_listeners; i++) {
			if (listeners[i])
				inotify_rm_watch(inotify_fd, listeners[i]->wd);
		}
		
		free(listeners);
		listeners = 0;
		close(inotify_fd);
	}
}
