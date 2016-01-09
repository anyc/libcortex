
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdarg.h>

#include "core.h"
#include "socket.h"
#include "fanotify.h"
#include "sd_bus_notifications.h"

struct crtx_listener_base *fa;
struct crtx_listener_base *sock_list;
struct crtx_fanotify_listener listener;
struct socket_listener sock_listener;

// static char *actions[] = { "Yes", "yes", "No", "no" };

char *sendtypes[2] = { "cortex.socket.outbox", 0 };
char *recvtypes[2] = { "cortex.socket.inbox", 0 };

static void fanotify_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct fanotify_event_metadata *metadata;
	char *buf, *chosen_action;
	char title[32];
	char msg[1024];
	struct crtx_event *notif_event;
	struct crtx_dict *data, *actions_dict;
	size_t title_len, buf_len;
	
	metadata = (struct fanotify_event_metadata *) event->data.raw;
	
	if (metadata->mask & FAN_OPEN_PERM) {
// 	if (metadata->mask & FAN_OPEN) {
		struct fanotify_response access;
		
		fanotify_get_path(metadata->fd, &buf, 0, "Grant permission to open \"%s\"");
		
		snprintf(title, 32, "PID %"PRId32, metadata->pid);
		
// 		send_notification("", title, buf, actions, &chosen_action);
// 		printf("chosen %s\n", chosen_action);
		snprintf(msg, 1024, "%s %s", title, buf);
		
		actions_dict = crtx_create_dict("ssss",
			"", "grant", 5, DIF_DATA_UNALLOCATED,
			"", "Yes", 3, DIF_DATA_UNALLOCATED,
			"", "deny", 4, DIF_DATA_UNALLOCATED,
			"", "No", 2, DIF_DATA_UNALLOCATED
			);
		
		title_len = strlen(title);
		buf_len = strlen(buf);
		data = crtx_create_dict("ssD",
			"title", stracpy(title, &title_len), title_len, 0,
			"message", stracpy(buf, &buf_len), buf_len, 0,
			"actions", actions_dict, 0, 0
			);
		
// 		notif_event = create_event(CRTX_EVT_NOTIFICATION, msg, strlen(msg)+1);
		notif_event = create_event(CRTX_EVT_NOTIFICATION, 0, 0);
		notif_event->data.dict = data;
		
		reference_event_release(notif_event);
		
		notif_event->response_expected = 1;
		
// 		send_event(&sock_listener, notif_event);
		add_event(sock_listener.outbox, notif_event);
		
		printf("wait for response\n");
		wait_on_event(notif_event);
		
		if (!crtx_get_value(notif_event->response.dict, "action", &chosen_action, sizeof(chosen_action))) {
			printf("received invalid response\n");
			crtx_print_dict(notif_event->response.dict);
			
			free(buf);
			return;
		}
		
		access.fd = metadata->fd;
		
		if (!strcmp(chosen_action, "grant"))
			access.response = FAN_ALLOW;
		else
			access.response = FAN_DENY;
		
		dereference_event_release(notif_event);
		
		/* Write response in the fanotify fd! */
		write(listener.fanotify_fd, &access, sizeof(access));
	}
	
	free(buf);
}

void init() {
	struct crtx_task * fan_handle_task;
	
	printf("starting fanotify example plugin\n");
	
	sock_listener.ai_family = AF_INET;
	sock_listener.type = SOCK_STREAM;
	sock_listener.protocol = IPPROTO_TCP;
	sock_listener.host = "localhost";
	sock_listener.service = "1234";
	
	sock_listener.send_types = sendtypes;
	sock_listener.recv_types = recvtypes;
	
	sock_list = create_listener("socket_client", &sock_listener);
	if (!sock_list) {
		printf("cannot create sock_listener\n");
		return;
	}
	
	
	listener.init_flags = FAN_CLASS_PRE_CONTENT;
// 	listener.init_flags = FAN_CLASS_NOTIF;
	listener.event_f_flags = O_RDONLY;
	listener.mark_flags = FAN_MARK_ADD; // | FAN_MARK_MOUNT;
	listener.mask = FAN_OPEN_PERM | FAN_EVENT_ON_CHILD; // CONFIG_FANOTIFY_ACCESS_PERMISSIONS
// 	listener.mask = FAN_OPEN | FAN_EVENT_ON_CHILD; // CONFIG_FANOTIFY_ACCESS_PERMISSIONS
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