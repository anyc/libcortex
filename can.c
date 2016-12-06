
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "can.h"

void crtx_free_can_listener(struct crtx_listener_base *data) {
	struct crtx_can_listener *clist;
	
	clist = (struct crtx_can_listener*) data;
	
	
}

struct crtx_listener_base *crtx_new_can_listener(void *options) {
	struct crtx_can_listener *clist;
	
	clist = (struct crtx_can_listener *) options;
	
	
	clist->>sockfd = socket(PF_CAN, SOCK_RAW, clist->protocol);
	
	if (clist->interface_name) {
		struct ifreq ifr;
		
		if (strlen(clist->interface_name) >= IFNAMSIZ-1) {
			fprintf(stderr, "too long interface name\n");
			return -1;
		}
		
		strcpy(ifr.ifr_name, clist->interface_name);
		r = ioctl(clist->>sockfd, SIOCGIFINDEX, &ifr);
		if (r != 0) {
			fprintf(stderr, "ioctl failed: %s\n", strerror(errno));
			return r;
		}
		
		clist->addr.can_family = AF_CAN;
		clist->addr.can_ifindex = ifr.ifr_ifindex;
	}
	
	r = bind(clist->sockfd, (struct sockaddr *)&clist->addr, sizeof(clist->addr));
	if (r != 0) {
		fprintf(stderr, "bind failed: %s\n", strerror(errno));
		return r;
	}
	
	
	clist->parent.el_payload.fd = can_monitor_get_fd(clist->monitor);
	clist->parent.el_payload.data = clist;
	clist->parent.el_payload.event_handler = &can_fd_event_handler;
	clist->parent.el_payload.event_handler_name = "can fd handler";
	
	clist->parent.free = &crtx_free_can_listener;
	new_eventgraph(&clist->parent.graph, "can", can_msg_etype);
	
	return &clist->parent;
}

void crtx_can_init() {
}

void crtx_can_finish() {
}
