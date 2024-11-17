/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 * fanotify enables monitoring of filesystem activity
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "intern.h"
#include "core.h"
#include "fanotify.h"
#include "threads.h"

#ifndef CRTX_TEST

void crtx_fanotify_get_path(int fd, char **path, size_t *length, char *format) {
	char fdpath[32];
	char buf[PATH_MAX];
	ssize_t linklen;
	size_t slen;
	
	sprintf(fdpath, "/proc/self/fd/%d", fd);
	
	linklen = readlink(fdpath, buf, PATH_MAX);
	if (linklen == -1) {
		printf("readlink failed\n");
		*path = 0;
		return;
	}
	buf[linklen] = '\0';
	
	if (!format) {
		*path = (char*) malloc(linklen+1);
		sprintf(*path, "%s", buf);
		
		slen = linklen;
	} else {
		slen = strlen(format)+linklen;
		*path = (char*) malloc(slen+1);
		snprintf(*path, slen+1, format, buf);
	}
	
	if (length)
		*length = slen;
}

void process_fanotify(struct crtx_fanotify_listener *fal) {
	char buf[4096];
	ssize_t buflen;
	struct fanotify_event_metadata *metadata, *copy;
	struct crtx_event *event;
	
	
	buflen = read(fal->fanotify_fd, buf, sizeof(buf));
	
	if (fal->stop)
		return;
	
	// FAN_EVENT_OK will check the size
	metadata = (struct fanotify_event_metadata*) &buf;
	
	while(FAN_EVENT_OK(metadata, buflen)) {
		if (metadata->mask & FAN_Q_OVERFLOW) {
			CRTX_ERROR("fanotify queue overflow\n");
			continue;
		}
		
		crtx_create_event(&event);
		if (fal->base.graph->descriptions)
			event->description = fal->base.graph->descriptions[0];
		
		copy = (struct fanotify_event_metadata*) calloc(1, sizeof(struct fanotify_event_metadata));
		memcpy(copy, metadata, sizeof(struct fanotify_event_metadata));
		
		crtx_event_set_raw_data(event, 'p', copy, sizeof(struct fanotify_event_metadata), 0);
		
		crtx_add_event(fal->base.graph, event);
		
		// NOTE after processing the event, the fd has to be closed!
		// close(metadata->fd);
		
		metadata = FAN_EVENT_NEXT(metadata, buflen);
	}
}

static char fanotify_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	process_fanotify((struct crtx_fanotify_listener*) userdata);
	
	return 0;
}

void *fanotify_tmain(void *data) {
	struct crtx_fanotify_listener *fal;
	
	fal = (struct crtx_fanotify_listener*) data;
	
	while (!fal->stop) {
		process_fanotify(fal);
	}
	
	return 0;
}

void crtx_shutdown_fanotify_listener(struct crtx_listener_base *lstnr) {
	struct crtx_fanotify_listener *falist = (struct crtx_fanotify_listener *) lstnr;
	
	close(falist->fanotify_fd);
}

static void stop_thread(struct crtx_thread *thread, void *data) {
	struct crtx_fanotify_listener *falist;
	
	CRTX_DBG("stopping fanotify\n");
	
	falist = (struct crtx_fanotify_listener*) data;
	
	falist->stop = 1;
	if (falist->fanotify_fd >= 0)
		close(falist->fanotify_fd);
	
	crtx_threads_interrupt_thread(thread);
}

struct crtx_listener_base *crtx_setup_fanotify_listener(void *options) {
	struct crtx_fanotify_listener *falist;
	enum crtx_processing_mode mode;
	int ret;
	
	falist = (struct crtx_fanotify_listener *) options;
	
	falist->fanotify_fd = fanotify_init(falist->init_flags, falist->event_f_flags);
	
	if (falist->fanotify_fd == -1) {
		printf("fanotify initialization failed: %s\n", strerror(errno));
		return 0;
	}
	
	ret = fanotify_mark(falist->fanotify_fd,
		falist->mark_flags, // FAN_MARK_ADD | FAN_MARK_MOUNT,
		falist->mask, // FAN_OPEN | FAN_EVENT_ON_CHILD,
		falist->dirfd, // AT_FDCWD, 
		falist->path // (char*) options
		);
	if (ret != 0) {
		printf("fanotify_mark failed: %d %s\n", errno, strerror(errno));
		return 0;
	}
	
	falist->base.shutdown = &crtx_shutdown_fanotify_listener;
	
	// get default mode or use NONE as fallback
	mode = crtx_get_mode(CRTX_PREFER_NONE);
	
	if (mode == CRTX_PREFER_THREAD) {
		falist->base.thread_job.fct = &fanotify_tmain;
		falist->base.thread_job.fct_data = falist;
		falist->base.thread_job.do_stop = &stop_thread;
	} else
	if (mode == CRTX_PREFER_ELOOP) {
		crtx_evloop_init_listener(&falist->base,
			&falist->fanotify_fd, 0,
			CRTX_EVLOOP_READ,
			0,
			&fanotify_event_handler,
			falist,
			0, 0
			);
	}
	
	return &falist->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(fanotify)

void crtx_fanotify_init() {
}

void crtx_fanotify_finish() {
}

#else /* CRTX_TEST */

#include <inttypes.h>

struct crtx_fanotify_listener lstnr;

static char event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct fanotify_event_metadata *metadata;
	char *buf;
	char title[32];
	struct crtx_dict *data, *actions_dict;
	size_t title_len, buf_len;
	struct crtx_fanotify_listener *lstnr;
	
	
	metadata = (struct fanotify_event_metadata *) event->data.pointer;
	lstnr = (struct crtx_fanotify_listener*) userdata;
	
	if ( (metadata->mask & FAN_OPEN_PERM) || (metadata->mask & FAN_OPEN) ) {
		// replace %s with the file path and store the string in buf
		crtx_fanotify_get_path(metadata->fd, &buf, 0, "Grant permission to open \"%s\"");
		
		// write the process id into the title
		snprintf(title, 32, "PID %"PRId32, metadata->pid);
		
		if (metadata->mask & FAN_OPEN_PERM) {
			// create options that are shown to a user
			// format: action_id, action message, ...
			actions_dict = crtx_create_dict("ssss",
				"", "grant", 5, CRTX_DIF_DONT_FREE_DATA,
				"", "Yes", 3, CRTX_DIF_DONT_FREE_DATA,
				"", "deny", 4, CRTX_DIF_DONT_FREE_DATA,
				"", "No", 2, CRTX_DIF_DONT_FREE_DATA
				);
		} else {
			// fanotify does not expect a response
			actions_dict = 0;
		}
		
		// create the payload of the event
		title_len = strlen(title);
		buf_len = strlen(buf);
		data = crtx_create_dict("ssD",
			"title", crtx_stracpy(title, &title_len), title_len, 0,
			"message", crtx_stracpy(buf, &buf_len), buf_len, 0,
			"actions", actions_dict, 0, 0
			);
		
		crtx_print_dict(data);
		
		crtx_dict_unref(data);
		
		if (metadata->mask & FAN_OPEN_PERM) {
			struct fanotify_response access;
			ssize_t srv;
			char *line;
			size_t len;
			
			
			access.fd = metadata->fd;
			
			printf("Type \"grant\" to grant access:\n");
			line = 0;
			srv = getline(&line, &len, stdin);
			
			if (srv > 0) {
				if (!strncmp(line, "grant", 5) || !strncmp(line, "Yes", 5))
					access.response = FAN_ALLOW;
				else
					access.response = FAN_DENY;
				
				write(lstnr->fanotify_fd, &access, sizeof(access));
			}
			
			free(line);
		}
		
		close(metadata->fd);
	}
	
	free(buf);
	
	return 0;
}

int test_main(int argc, char **argv) {
	int rv;
	
	
	memset(&lstnr, 0, sizeof(struct crtx_fanotify_listener));
	
	if (argc > 1) {
		lstnr.path = argv[1];
	} else {
		fprintf(stderr, "Usage: %s <path> [mask1,mask2,...]\n", argv[0]);
		return 1;
	}
	
	// request permission before opening a file
	lstnr.init_flags = FAN_CLASS_PRE_CONTENT;
	lstnr.mask = FAN_OPEN_PERM | FAN_EVENT_ON_CHILD;
	
	// just notify if a file has been opened
	// 	lstnr.init_flags = FAN_CLASS_NOTIF;
	// 	lstnr.mask = FAN_OPEN | FAN_EVENT_ON_CHILD;
	
	lstnr.mark_flags = FAN_MARK_ADD;
	// lstnr.mark_flags |= FAN_MARK_MOUNT; // mark complete mount point
	
	lstnr.event_f_flags = O_RDONLY;
	lstnr.dirfd = AT_FDCWD;
	
	rv = crtx_setup_listener("fanotify", &lstnr);
	if (rv) {
		fprintf(stderr, "cannot create fanotify lstnr\n");
		return 0;
	}
	
	// add function that will process a fanotify event
	crtx_create_task(lstnr.base.graph, 0, "event_handler", &event_handler, &lstnr.base);
	
	crtx_start_listener(&lstnr.base);
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(test_main);

#endif /* CRTX_TEST */
