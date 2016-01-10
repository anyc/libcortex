
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>

#include "core.h"
#include "socket.h"
#include "sd_bus_notifications.h"

struct crtx_listener_base *sock_list, *notify_list;
struct crtx_socket_listener sock_listener;
struct crtx_sd_bus_notification_listener notify_listener;

void init() {
	printf("starting notifyd example plugin\n");
	
	sock_listener.ai_family = AF_INET;
	sock_listener.type = SOCK_STREAM;
	sock_listener.protocol = IPPROTO_TCP;
	sock_listener.host = "localhost";
	sock_listener.service = "1234";
	
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
}