
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include "core.h"
#include "socket.h"
#include "fanotify.h"
#include "sd_bus_notifications.h"

struct listener *fa;
struct listener *sock_list;
struct fanotify_listener listener;
struct socket_listener sock_listener;

// static char *actions[] = { "Yes", "yes", "No", "no" };

static void fanotify_event_handler(struct event *event, void *userdata, void **sessiondata) {
	struct fanotify_event_metadata *metadata;
	char *buf, *chosen_action;
	char title[32];
	char msg[1024];
	struct event *notif_event;
	
	metadata = (struct fanotify_event_metadata *) event->data;
	
// 	if (metadata->mask & FAN_OPEN_PERM) {
	if (metadata->mask & FAN_OPEN) {
// 		struct fanotify_response access;
		
		fanotify_get_path(metadata->fd, &buf, 0, "Grant permission to open \"%s\"");
		
		snprintf(title, 32, "PID %"PRId32, metadata->pid);
		
// 		send_notification("", title, buf, actions, &chosen_action);
// 		printf("chosen %s\n", chosen_action);
		snprintf(msg, 1024, "%s %s", title, buf);
		
		notif_event = new_event();
		notif_event->type = "sd-bus.org.freedesktop.Notifications.ActionInvoked";
		notif_event->data = msg;
		notif_event->data_size = strlen(msg);
		
		send_event(&sock_listener, notif_event);
		
		
// 		access.fd = metadata->fd;
// 		
// // 		access.response = FAN_DENY;
// 		access.response = FAN_ALLOW;
// 		
// 		/* Write response in the fanotify fd! */
// 		write(listener.fanotify_fd, &access, sizeof (access));
	}
	
	free(buf);
}

void init() {
	struct event_task * fan_handle_task;
	
	printf("starting fanotify example plugin\n");
	
	sock_listener.ai_family = AF_INET;
	sock_listener.type = SOCK_STREAM;
	sock_listener.protocol = IPPROTO_TCP;
	sock_listener.host = "localhost";
	sock_listener.service = "1234";
	
	sock_list = create_listener("socket_client", &sock_listener);
	if (!sock_list) {
		printf("cannot create sock_listener\n");
		return;
	}
	
	
// 	listener.init_flags = FAN_CLASS_PRE_CONTENT;
	listener.init_flags = FAN_CLASS_NOTIF;
	listener.event_f_flags = O_RDONLY;
	listener.mark_flags = FAN_MARK_ADD; // | FAN_MARK_MOUNT;
// 	listener.mask = FAN_OPEN_PERM | FAN_EVENT_ON_CHILD; // CONFIG_FANOTIFY_ACCESS_PERMISSIONS
	listener.mask = FAN_OPEN | FAN_EVENT_ON_CHILD; // CONFIG_FANOTIFY_ACCESS_PERMISSIONS
	listener.dirfd = AT_FDCWD;
	listener.path = "."; // current working directory
	
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