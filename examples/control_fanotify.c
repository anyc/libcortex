
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>

#include "core.h"
#include "fanotify.h"
#include "sd_bus_notifications.h"

struct listener *fa;
struct fanotify_listener listener;

static char *actions[] = { "Yes", "yes", "No", "no" };

static void fanotify_event_handler(struct event *event, void *userdata, void **sessiondata) {
	struct fanotify_event_metadata *metadata;
	char *buf, *chosen_action;
	char title[32];
	
	metadata = (struct fanotify_event_metadata *) event->data;
	
	if (metadata->mask & FAN_OPEN_PERM) {
		struct fanotify_response access;
		
		fanotify_get_path(metadata->fd, &buf, 0, "Grant permission to open \"%s\"");
		
		snprintf(title, 32, "PID %"PRId32, metadata->pid);
		
		send_notification("", title, buf, actions, &chosen_action);
		printf("chosen %s\n", chosen_action);
		
		access.fd = metadata->fd;
		
// 		access.response = FAN_DENY;
		access.response = FAN_ALLOW;
		
		/* Write response in the fanotify fd! */
		write(listener.fanotify_fd, &access, sizeof (access));
	}
	
	free(buf);
}

void init() {
	struct event_task * fan_handle_task;
	
	printf("starting fanotify example plugin\n");
	
	listener.init_flags = FAN_CLASS_PRE_CONTENT;
	listener.event_f_flags = O_RDONLY;
	listener.mark_flags = FAN_MARK_ADD; // | FAN_MARK_MOUNT;
	listener.mask = FAN_OPEN_PERM | FAN_EVENT_ON_CHILD; // CONFIG_FANOTIFY_ACCESS_PERMISSIONS
	listener.dirfd = AT_FDCWD;
	listener.path = "/tmp/"; // current working directory
	send_notification("", "test", "asd", actions, 0);
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