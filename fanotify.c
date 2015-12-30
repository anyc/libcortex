
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "core.h"
#include "fanotify.h"

char *fanotify_msg_etype[] = { FANOTIFY_MSG_ETYPE, 0 };

void fanotify_get_path(int fd, char **path, size_t *length, char *format) {
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

void *fanotify_tmain(void *data) {
	char buf[4096];
	ssize_t buflen;
	struct fanotify_event_metadata *metadata;
	struct event *event;
	struct fanotify_listener *fal;
	
	fal = (struct fanotify_listener*) data;
	
	while (1) {
		buflen = read(fal->fanotify_fd, buf, sizeof(buf));
		
		// FAN_EVENT_OK will check the size
		metadata = (struct fanotify_event_metadata*)&buf;
		
		while(FAN_EVENT_OK(metadata, buflen)) {
			if (metadata->mask & FAN_Q_OVERFLOW) {
				printf("Queue overflow!\n");
				continue;
			}
			
			event = new_event();
			event->type = fal->parent.graph->types[0];
			event->data = metadata;
			event->data_size = sizeof(struct fanotify_event_metadata);
			
			add_event(fal->parent.graph, event);
			
			wait_on_event(event);
			
			close(metadata->fd);
			
			metadata = FAN_EVENT_NEXT(metadata, buflen);
		}
	}
	
	return 0;
}

void free_fanotify_listener(void *data) {
	struct fanotify_listener *falist = (struct fanotify_listener *) falist;
	
	free_eventgraph(falist->parent.graph);
	
// 	pthread_join(falist->thread, 0);
	
	free(falist);
}

struct listener *new_fanotify_listener(void *options) {
	struct fanotify_listener *falist;
	int ret;
	
	falist = (struct fanotify_listener *) options;
	
	falist->fanotify_fd = fanotify_init(falist->init_flags, falist->event_f_flags);
	
	if (falist->fanotify_fd == -1) {
		printf("fanotify initialization failed: %s\n", strerror(errno));
		return 0;
	}
	
	ret = fanotify_mark(falist->fanotify_fd,
				falist->mark_flags, // FAN_MARK_ADD | FAN_MARK_MOUNT,
				falist->mask, // FAN_OPEN | FAN_EVENT_ON_CHILD,
				falist->dirfd, // AT_FDCWD, 
				falist->path); // (char*) options);
	if (ret != 0) {
		printf("fanotify_mark failed: %d %s\n", errno, strerror(errno));
		return 0;
	}
	
	falist->parent.free = &free_fanotify_listener;
	
	new_eventgraph(&falist->parent.graph, fanotify_msg_etype);
	
	pthread_create(&falist->thread, NULL, fanotify_tmain, falist);
	
	return &falist->parent;
}

void crtx_fanotify_init() {
}

void crtx_fanotify_finish() {
}