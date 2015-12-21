
#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "fanotify.h"

struct listener *fa;
struct fanotify_listener listener;

static void fanotify_event_handler(struct event *event, void *userdata, void **sessiondata) {
	struct fanotify_event_metadata *metadata;
	char *buf;
	
	metadata = (struct fanotify_event_metadata *) event->data;
	
	fanotify_get_path(metadata->fd, &buf, 0);
	printf("rec %s\n", buf);
	
	free(buf);
}

void init() {
	struct event_task * fan_handle_task;
	
	printf("starting fanotify example plugin\n");
	
	listener.init_flags = FAN_CLASS_NOTIF;
	listener.event_f_flags = O_RDONLY;
	listener.mark_flags = FAN_MARK_ADD | FAN_MARK_MOUNT;
	listener.mask = FAN_OPEN | FAN_EVENT_ON_CHILD;
	listener.dirfd = AT_FDCWD;
	listener.path = "/";
	
	fa = create_listener("fanotify", &listener);
	if (!fa) {
		printf("cannot create fanotify listener\n");
		return;
	}
	
	fan_handle_task = new_task();
	fan_handle_task->id = "fanotify_event_handler";
	fan_handle_task->handle = &fanotify_event_handler;
	fan_handle_task->userdata = fa;
	add_task(fa->graph, fan_handle_task);
}

void finish() {
	free_listener(fa);
}