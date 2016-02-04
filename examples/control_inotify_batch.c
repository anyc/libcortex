
/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "core.h"
#include "inotify.h"
#include "sd_bus_notifications.h"
#include "dict.h"

static struct crtx_listener_base *in_base;
static struct crtx_inotify_listener listener;
static void *notifier_data = 0;


struct crtx_dict_transformation to_notification[] = {
	{ "title", 's', 0, "Inotify" },
	{ "message", 's', 0, "Event \"%[mask](%[*]s)\" for file \"%[name]s\"" }
};

struct crtx_transform_dict_handler transform_task;

char init() {
	char *path;
	
	printf("starting inotify example plugin\n");
	
	crtx_init_notification_listeners(&notifier_data);
	
	path = getenv("INOTIFY_PATH");
	if (!path)
		path = ".";
	
	// setup inotify
	listener.path = path;
	listener.mask = IN_CREATE | IN_DELETE;
	
	// create a listener (queue) for inotify events
	in_base = create_listener("inotify", &listener);
	if (!in_base) {
		printf("cannot create inotify listener\n");
		return 0;
	}
	
	// setup a task that will process the inotify events
// 	crtx_create_task(in_base->graph, 0, "inotify_event_handler", &inotify_event_handler, in_base);
	
	transform_task.signature = "ss";
	transform_task.transformation = to_notification;
	transform_task.graph = 0;
	transform_task.type = CRTX_EVT_NOTIFICATION;
	
	crtx_create_transform_task(in_base->graph, "inotify_to_notification", &transform_task);
	
	return 1;
}

void finish() {
	crtx_finish_notification_listeners(notifier_data);
	
	free_listener(in_base);
}