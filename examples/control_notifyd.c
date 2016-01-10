
/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 * This example starts a notification daemon for the current user. It opens a
 * server socket and waits for notification events. If such an event is
 * recevied, it sends a notification to the notification D-BUS service.
 *
 * See control_fanotify.c for an example that sends notification events and
 * https://developer.gnome.org/notification-spec/ for more information on
 * the D-BUS service.
 *
 * This example reads the following environment variables:
 *
 *  - NOTIFIER_PATH: path for the socket of the notification daemon
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>


#include "core.h"
#include "socket.h"
#include "sd_bus_notifications.h"

#define PREFIX "notification_daemon"

struct crtx_listener_base *sock_list, *notify_list;
struct crtx_socket_listener sock_listener;
struct crtx_sd_bus_notification_listener notify_listener;

char *sock_path = 0;

void init() {
	printf("starting notifyd example plugin\n");
	
	// open a TCPv4 server socket,
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
		
		user = get_username();
		
		// default path: /var/run/cortexd/notification_daemon_$user
		
		sock_path = getenv("NOTIFIER_PATH");
		if (!sock_path) {
			sock_path = (char*) malloc(strlen(CRTX_UNIX_SOCKET_DIR) + 1 + strlen(PREFIX) + 1 + strlen(user) + 1);
			sprintf(sock_path, "%s/%s_%s", CRTX_UNIX_SOCKET_DIR, PREFIX, user);
		}
		
		sock_listener.ai_family = AF_UNIX;
		sock_listener.type = SOCK_STREAM;
		sock_listener.service = sock_path;
	}
	
	sock_list = create_listener("socket_server", &sock_listener);
	if (!sock_list) {
		printf("cannot create fanotify listener\n");
		return;
	}
	
	notify_list = create_listener("sd_bus_notification", &notify_listener);
	if (!notify_list) {
		printf("cannot create fanotify listener\n");
		return;
	}
}

void finish() {
	free_listener(sock_list);
	free_listener(notify_list);
	
	if (sock_path)
		free(sock_path);
}