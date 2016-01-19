
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "core.h"
#include "inotify.h"
#include "threads.h"

static int inotify_fd = -1;
char *inotify_msg_etype[] = { INOTIFY_MSG_ETYPE, 0 };

#define INOFITY_BUFLEN (sizeof(struct inotify_event) + NAME_MAX + 1)

void *inotify_tmain(void *data) {
	char buffer[INOFITY_BUFLEN];
	ssize_t length;
	size_t i;
	
	struct crtx_event *event;
	struct crtx_inotify_listener *fal;
	struct inotify_event *in_event;
	
	fal = (struct crtx_inotify_listener*) data;
	
	while (!fal->stop) {
		length = read(inotify_fd, buffer, INOFITY_BUFLEN);
		
		if (fal->stop)
			break;
		
		if (length < 0) {
			printf("inotify read failed\n");
			break;
		}
		
		i=0;
		while (i < length) {
			in_event = (struct inotify_event *) &buffer[i];
			
			event = create_event(fal->parent.graph->types[0], in_event, sizeof(struct inotify_event) + in_event->len);
			
			reference_event_release(event);
			
			add_event(fal->parent.graph, event);
			
			wait_on_event(event);
			
			event->data.raw = 0;
			
			dereference_event_release(event);
			
			i += sizeof(struct inotify_event) + in_event->len;
		}
	}
	
	return 0;
}

void crtx_free_inotify_listener(struct crtx_listener_base *data) {
// 	struct crtx_inotify_listener *inlist = (struct crtx_inotify_listener *) data;
}

static void stop_thread(struct crtx_thread *thread, void *data) {
	struct crtx_inotify_listener *inlist;
	
	inlist = (struct crtx_inotify_listener*) data;
	
	inlist->stop = 1;
	inotify_rm_watch(inotify_fd, inlist->wd);
	close(inotify_fd);
}

struct crtx_listener_base *crtx_new_inotify_listener(void *options) {
	struct crtx_inotify_listener *inlist;
	struct crtx_thread *t;
	
	inlist = (struct crtx_inotify_listener*) options;
	
	if (inotify_fd == -1) {
		inotify_fd = inotify_init();
		
		if (inotify_fd == -1) {
			printf("inotify initialization failed\n");
			return 0;
		}
	}
	
	inlist->wd = inotify_add_watch(inotify_fd, inlist->path, inlist->mask);
	if (inlist->wd == -1) {
		printf("inotify_add_watch(\"%s\") failed with %d: %s\n", (char*) options, errno, strerror(errno));
		return 0;
	}
	
	inlist->parent.free = &crtx_free_inotify_listener;
	
	new_eventgraph(&inlist->parent.graph, 0, inotify_msg_etype);
	
	t = get_thread(inotify_tmain, inlist, 1);
	t->do_stop = &stop_thread;
	
	return &inlist->parent;
}

void crtx_inotify_init() {
}

void crtx_inotify_finish() {
}
