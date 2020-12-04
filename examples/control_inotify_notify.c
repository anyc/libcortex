/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 * This example creates a D-BUS notification every time a file or directory
 * is created or deleted using inotify.
 * 
 * This example considers the following environment variables:
 *
 *  - INOTIFY_PATH: path that will be monitored (default: the current work
 *                  directory)
 */

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "core.h"
#include "inotify.h"
#include "sdbus_notifications.h"
#include "dict.h"
#include "intern.h"

static struct crtx_inotify_listener listener;
static void *notifier_data = 0;

/// this function sets up and sends a notification event
static void send_notification(char *msg) {
	struct crtx_event *notif_event;
	struct crtx_dict *data;
	size_t msg_len;
	int r;
	
	
	r = crtx_create_event(&notif_event);
	if (r) {
		CRTX_ERROR("crtx_create_event() failed: %s\n", strerror(-r));
		return;
	}
	notif_event->description = CRTX_EVT_NOTIFICATION;
	
	msg_len = 0;
	data = crtx_create_dict("ss",
			"title", "Inotify", strlen("Inotify"), CRTX_DIF_DONT_FREE_DATA,
			"message", crtx_stracpy(msg, &msg_len), msg_len, 0
			);
	notif_event->data.dict = data;
	
	add_raw_event(notif_event);
}

/// this function is called for every inotify event
static char inotify_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct inotify_event *in_event;
	char buf[1024];
	
	in_event = (struct inotify_event *) event->data.pointer;
	if (in_event->len) {
		if (in_event->mask & IN_CREATE) {
			if (in_event->mask & IN_ISDIR) {
				printf("New directory %s created\n", in_event->name);
				snprintf(buf, 1024, "New directory %s created", in_event->name);
				send_notification(buf);
			}
			else {
				printf("New file %s created\n", in_event->name);
				snprintf(buf, 1024, "New file %s created", in_event->name);
				send_notification(buf);
			}
		} else
		if (in_event->mask & IN_DELETE) {
			if (in_event->mask & IN_ISDIR) {
				printf("Directory %s deleted\n", in_event->name);
				snprintf(buf, 1024, "Directory %s deleted", in_event->name);
				send_notification(buf);
			}
			else {
				printf("File %s deleted\n", in_event->name);
				snprintf(buf, 1024, "File %s deleted", in_event->name);
				send_notification(buf);
			}
		}
	}
	
	return 1;
}

char init() {
	char *path;
	int r;
	
	
	printf("starting inotify example plugin\n");
	
	crtx_init_notification_listeners(&notifier_data);
	
	path = getenv("INOTIFY_PATH");
	if (!path)
		path = ".";
	
	// setup inotify
	listener.path = path;
	listener.mask = IN_CREATE | IN_DELETE;
	
	// create a listener (queue) for inotify events
	r = crtx_setup_listener("inotify", &listener);
	if (r) {
		CRTX_ERROR("cannot create inotify listener\n");
		return 0;
	}
	
	// setup a task that will process the inotify events
	crtx_create_task(listener.base.graph, 0, "inotify_event_handler", &inotify_event_handler, &listener.base);
	
	crtx_start_listener(&listener.base);
	
	crtx_loop();
	
	crtx_finish_notification_listeners(notifier_data);
	
	crtx_free_listener(&listener.base);
}

CRTX_TEST_MAIN(init);
