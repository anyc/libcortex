
/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 * This example sets up fanotify to ask for permission before a file in the
 * current work directory can be opened. To ask for permission, this example
 * sends a notification event over a socket to a notification daemon. Please
 * see control_notifyd.c for an example of such a daemon.
 *
 * Fanotify requires root privileges (CAP_SYS_ADMIN). Hence, this example
 * usually needs to be executed as root while the notification daemon can run
 * with the privileges of a local user.
 *
 * This example needs a kernel with CONFIG_FANOTIFY_ACCESS_PERMISSIONS enabled!
 *
 * See control_inotify.c for a simpler way to receive notifications for file
 * operations.
 * 
 * This example reads the following environment variables:
 *
 *  - FANOTIFY_USER: contains the username of the user who will be asked for
 *                 permission
 *  - FANOTIFY_PATH: the path that will be monitored
 *  - NOTIFIER_PATH: path to the socket of the notification daemon
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdarg.h>


#include "core.h"
#include "dict.h"
#include "socket.h"
#include "fanotify.h"
#include "sd_bus_notifications.h"

#define PREFIX "notification_daemon"

struct crtx_listener_base *fa;
struct crtx_listener_base *sock_list;
struct crtx_fanotify_listener listener;
struct crtx_socket_listener sock_listener;

char *sock_path = 0;

/// this function will be called for each fanotify event
static void fanotify_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct fanotify_event_metadata *metadata;
	char *buf, *chosen_action;
	char title[32];
	struct crtx_event *notif_event;
	struct crtx_dict *data, *actions_dict;
	size_t title_len, buf_len;
	
	metadata = (struct fanotify_event_metadata *) event->data.raw;
	
	if ( (metadata->mask & FAN_OPEN_PERM) || (metadata->mask & FAN_OPEN) ) {
		// replace %s with the file path and store the string in buf
		crtx_fanotify_get_path(metadata->fd, &buf, 0, "Grant permission to open \"%s\"");
		
		// write the process id into the title
		snprintf(title, 32, "PID %"PRId32, metadata->pid);
		
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
			// fanotify does not expect a response
			actions_dict = 0;
		}
		
		// create the payload of the event
		title_len = strlen(title);
		buf_len = strlen(buf);
		data = crtx_create_dict("ssD",
			"title", stracpy(title, &title_len), title_len, 0,
			"message", stracpy(buf, &buf_len), buf_len, 0,
			"actions", actions_dict, 0, 0
			);
		
		notif_event = create_event(CRTX_EVT_NOTIFICATION, 0, 0);
		notif_event->data.dict = data;
		
		if (metadata->mask & FAN_OPEN_PERM) {
			// indicate that we are interested in a response to this event
			reference_event_release(notif_event);
			notif_event->response_expected = 1;
		}
		
		// process event
		add_event(sock_listener.outbox, notif_event);
		
		if (metadata->mask & FAN_OPEN_PERM) {
			struct fanotify_response access;
			
			wait_on_event(notif_event);
			
			if (!crtx_get_value(notif_event->response.dict, "action", 's', &chosen_action, sizeof(chosen_action))) {
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
	}
	
	free(buf);
}

char init() {
	char *fanotify_path;
	
	printf("starting fanotify example plugin\n");
	
	/*
	 * setup a socket listener
	 */
	
	// open a TCPv4 client socket,
	/*
	{
		sock_listener.ai_family = AF_INET;
		sock_listener.type = SOCK_STREAM;
		sock_listener.protocol = IPPROTO_TCP;
		sock_listener.host = "localhost";
		sock_listener.service = "1234";
	}
	*/
	
	// or open a unix socket
	{
		char *user;
		
		user = getenv("FANOTIFY_USER");
		if (!user) {
			printf("FANOTIFY_USER not set!\n");
			user = get_username();
		}
		
		// default path: /var/run/cortexd/notification_daemon_$user
		
		sock_path = getenv("NOTIFIER_PATH");
		if (!sock_path) {
			sock_path = (char*) malloc(strlen(CRTX_UNIX_SOCKET_DIR) + 1 + strlen(PREFIX) + 1 + strlen(user) + 1);
			sprintf(sock_path, "%s/%s_%s", CRTX_UNIX_SOCKET_DIR, PREFIX, user);
		}
		
		memset(&sock_listener, 0, sizeof(struct crtx_socket_listener));
		sock_listener.ai_family = AF_UNIX;
		sock_listener.type = SOCK_STREAM;
		sock_listener.service = sock_path;
	}
	
	sock_list = create_listener("socket_client", &sock_listener);
	if (!sock_list) {
		printf("cannot create sock_listener\n");
		return 0;
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
	
	fanotify_path = getenv("FANOTIFY_PATH");
	if (fanotify_path)
		listener.path = fanotify_path;
	else
		listener.path = "."; // current working directory
	
	fa = create_listener("fanotify", &listener);
	if (!fa) {
		printf("cannot create fanotify listener\n");
		return 0;
	}
	
	// add function that will process a fanotify event
	crtx_create_task(fa->graph, 0, "fanotify_event_handler", &fanotify_event_handler, fa);
	
	printf("fanotify path \"%s\", sending events to \"%s\"\n", fanotify_path, sock_path);
	
	return 1;
}

void finish() {
	free_listener(sock_list);
	free_listener(fa);
	
	if (sock_path)
		free(sock_path);
}