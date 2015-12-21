
#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "inotify.h"

struct listener *fa;
struct inotify_listener listener;

static void inotify_event_handler(struct event *event, void *userdata, void **sessiondata) {
	struct inotify_event *in_event;
	
	in_event = (struct inotify_event *) event->data;
	if (in_event->len) {
		if ( in_event->mask & IN_CREATE ) {
			if ( in_event->mask & IN_ISDIR ) {
				printf( "New directory %s created.\n", in_event->name );
			}
			else {
				printf( "New file %s created.\n", in_event->name );
			}
		}
		else if ( in_event->mask & IN_DELETE ) {
			if ( in_event->mask & IN_ISDIR ) {
				printf( "Directory %s deleted.\n", in_event->name );
			}
			else {
				printf( "File %s deleted.\n", in_event->name );
			}
		}
	}
}


void init() {
	struct event_task * fan_handle_task;
	
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