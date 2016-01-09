
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>

#include "core.h"
#include "socket.h"
#include "sd_bus_notifications.h"

struct listener *fa;
struct socket_listener sock_listener;
struct sd_bus_notifications_listener notify_listener;

// static void notifyd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct fanotify_event_metadata *metadata;
// 	char *buf, *chosen_action;
// 	char title[32];
// 	
// 	metadata = (struct fanotify_event_metadata *) event->data;
// 	
// 	if (metadata->mask & FAN_OPEN_PERM) {
// 		struct fanotify_response access;
// 		
// 		fanotify_get_path(metadata->fd, &buf, 0, "Grant permission to open \"%s\"");
// 		
// 		snprintf(title, 32, "PID %"PRId32, metadata->pid);
// 		
// 		send_notification("", title, buf, actions, &chosen_action);
// 		printf("chosen %s\n", chosen_action);
// 		
// 		access.fd = metadata->fd;
// 		
// 		// 		access.response = FAN_DENY;
// 		access.response = FAN_ALLOW;
// 		
// 		/* Write response in the fanotify fd! */
// 		write(listener.fanotify_fd, &access, sizeof (access));
// 	}
// 	
// 	free(buf);
// }

void init() {
// 	struct crtx_task * fan_handle_task;
	
	printf("starting notifyd example plugin\n");
	
	sock_listener.ai_family = AF_INET;
	sock_listener.type = SOCK_STREAM;
	sock_listener.protocol = IPPROTO_TCP;
	sock_listener.host = "localhost";
	sock_listener.service = "1234";
	
	fa = create_listener("socket_server", &sock_listener);
	if (!fa) {
		printf("cannot create fanotify listener\n");
		return;
	}
	
	fa = create_listener("sd_bus_notification", &notify_listener);
	if (!fa) {
		printf("cannot create fanotify listener\n");
		return;
	}
	
// 	fan_handle_task = new_task();
// 	fan_handle_task->id = "notifyd_event_handler";
// 	fan_handle_task->handle = &notifyd_event_handler;
// 	fan_handle_task->userdata = fa;
// 	add_task(fa->graph, fan_handle_task);
}

void finish() {
	free_listener(fa);
}