
/*
 * Mario Kicherer (dev@kicherer.org) 2016
 * 
 * This example sets up fanotify to ask for permission before a file in the
 * current work directory can be opened. The permission request is sent to
 * the notification graph and if the response is "grant", the permission to
 * open the file is granted.
 */

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

// CONFIG_FANOTIFY_ACCESS_PERMISSIONS

struct crtx_listener_base *fa;
struct crtx_listener_base *sock_list;
struct crtx_fanotify_listener listener;
struct crtx_socket_listener sock_listener;


static void fanotify_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct fanotify_event_metadata *metadata;
	char *buf, *chosen_action;
	char title[32];
	char msg[1024];
	struct crtx_event *notif_event;
	struct crtx_dict *data, *actions_dict;
	size_t title_len, buf_len;
	
	metadata = (struct fanotify_event_metadata *) event->data.raw;
	
	if ( (metadata->mask & FAN_OPEN_PERM) || (metadata->mask & FAN_OPEN) ) {
		struct fanotify_response access;
		
		// acts like sprintf to replace %s with the file path
		crtx_fanotify_get_path(metadata->fd, &buf, 0, "Grant permission to open \"%s\"");
		
		snprintf(title, 32, "PID %"PRId32, metadata->pid);
		snprintf(msg, 1024, "%s %s", title, buf);
		
		if (metadata->mask & FAN_OPEN_PERM) {
			// create options that are shown to a user
			// format: action_id, action message, ...
			actions_dict = crtx_create_dict("ssss",
				"", "grant", 5, DIF_DATA_UNALLOCATED,
				"", "Yes", 3, DIF_DATA_UNALLOCATED,
				"", "deny", 4, DIF_DATA_UNALLOCATED,
				"", "No", 2, DIF_DATA_UNALLOCATED
				);
		} else {
			actions_dict = 0;
		}
		
		title_len = strlen(title);
		buf_len = strlen(buf);
		data = crtx_create_dict("ssD",
			"title", stracpy(title, &title_len), title_len, 0,
			"message", stracpy(buf, &buf_len), buf_len, 0,
			"actions", actions_dict, 0, 0
			);
		
		notif_event = create_event(CRTX_EVT_NOTIFICATION, 0, 0);
		notif_event->data.dict = data;
		
		// indicate that we are interested in a response to this event
		reference_event_release(notif_event);
		notif_event->response_expected = 1;
		
		// process event
		add_event(sock_listener.outbox, notif_event);
		
		wait_on_event(notif_event);
		
		if (!crtx_get_value(notif_event->response.dict, "action", &chosen_action, sizeof(chosen_action))) {
			printf("received invalid response:\n");
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
		
		write(listener.fanotify_fd, &access, sizeof(access));
	}
	
	free(buf);
}

void init() {
	struct crtx_task * fan_handle_task;
	
	printf("starting fanotify example plugin\n");
	
	/*
	 * setup a socket listener that opens a TCPv4 connection to localhost:1234
	 */
	
	memset(&sock_listener, 0, sizeof(struct crtx_socket_listener));
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
	
	/*
	 * setup the fanotiy listener
	 */
	
	// request permission before opening a file
	listener.init_flags = FAN_CLASS_PRE_CONTENT;
	listener.mask = FAN_OPEN_PERM | FAN_EVENT_ON_CHILD;
	
	// just notify if a file has been opened
// 	listener.init_flags = FAN_CLASS_NOTIF;
// 	listener.mask = FAN_OPEN | FAN_EVENT_ON_CHILD;
	
	listener.mark_flags = FAN_MARK_ADD;
// 	listener.mark_flags |= FAN_MARK_MOUNT; // mark complete mount point
	
	listener.event_f_flags = O_RDONLY;
	listener.dirfd = AT_FDCWD;
	listener.path = "."; // current working directory
	
	fa = create_listener("fanotify", &listener);
	if (!fa) {
		printf("cannot create fanotify listener\n");
		return;
	}
	
	/*
	 * add function that will process a fanotify event
	 */
	
	fan_handle_task = new_task();
	fan_handle_task->id = "fanotify_event_handler";
	fan_handle_task->handle = &fanotify_event_handler;
	fan_handle_task->userdata = fa;
	add_task(fa->graph, fan_handle_task);
}

void finish() {
	free_listener(sock_list);
	free_listener(fa);
}