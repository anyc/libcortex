
#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "inotify.h"
#include "sd_bus_notifications.h"

struct listener *fa;
struct inotify_listener listener;

static void inotify_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct inotify_event *in_event;
	char buf[1024];
	
	in_event = (struct inotify_event *) event->data.raw;
	if (in_event->len) {
		if (in_event->mask & IN_CREATE) {
			if (in_event->mask & IN_ISDIR) {
				printf("New directory %s created\n", in_event->name);
				snprintf(buf, 1024, "New directory %s created", in_event->name);
				send_notification("", "Inotify", buf, 0, 0);
			}
			else {
				printf("New file %s created\n", in_event->name);
				snprintf(buf, 1024, "New file %s created", in_event->name);
				send_notification("", "Inotify", buf, 0, 0);
			}
		} else
		if (in_event->mask & IN_DELETE) {
			if (in_event->mask & IN_ISDIR) {
				printf("Directory %s deleted\n", in_event->name);
				snprintf(buf, 1024, "Directory %s deleted", in_event->name);
				send_notification("", "Inotify", buf, 0, 0);
			}
			else {
				printf("File %s deleted\n", in_event->name);
				snprintf(buf, 1024, "File %s deleted", in_event->name);
				send_notification("", "Inotify", buf, 0, 0);
			}
		}
	}
}


void init() {
	struct crtx_task * fan_handle_task;
	
	printf("starting inotify example plugin\n");
	
	listener.mask = IN_CREATE | IN_DELETE;
	listener.path = ".";
	
	fa = create_listener("inotify", &listener);
	if (!fa) {
		printf("cannot create inotify listener\n");
		return;
	}
	
	fan_handle_task = new_task();
	fan_handle_task->id = "inotify_event_handler";
	fan_handle_task->handle = &inotify_event_handler;
	fan_handle_task->userdata = fa;
	add_task(fa->graph, fan_handle_task);
}

void finish() {
	free_listener(fa);
}