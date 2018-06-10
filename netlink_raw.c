/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <linux/netlink.h>

#include "intern.h"
#include "core.h"
#include "netlink_raw.h"

// static void *netlink_raw_tmain(void *data) {
// 	struct crtx_netlink_raw_listener *nl_listener;
// 	int r;
// 	
// 	nl_listener = (struct crtx_netlink_raw_listener*) data;
// 	
// 	r = 0;
// 	while (!nl_listener->stop && r == 0) {
// 		r = nl_listener->read_cb(nl_listener, nl_listener->sockfd, nl_listener->read_cb_userdata);
// 	}
// 	
// 	return 0;
// }

static char netlink_el_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_netlink_raw_listener *nl_listener;
	struct crtx_evloop_callback *el_cb;
	
	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	
	nl_listener = (struct crtx_netlink_raw_listener*) el_cb->event_handler_data;
	
	nl_listener->read_cb(nl_listener, nl_listener->sockfd, nl_listener->read_cb_userdata);
	
	return 0;
}

// static void stop_thread(struct crtx_thread *t, void *data) {
// 	struct crtx_netlink_raw_listener *nl_listener;
// 	
// 	DBG("stopping netlink_raw listener\n");
// 	
// 	nl_listener = (struct crtx_netlink_raw_listener*) data;
// 	
// 	nl_listener->stop = 1;
// 	
// 	if (nl_listener->sockfd)
// 		shutdown(nl_listener->sockfd, SHUT_RDWR);
// }

struct crtx_listener_base *crtx_new_netlink_raw_listener(void *options) {
	struct crtx_netlink_raw_listener *nl_listener;
	struct sockaddr_nl addr;
	
	nl_listener = (struct crtx_netlink_raw_listener*) options;
	
	if ((nl_listener->sockfd = socket(PF_NETLINK, SOCK_RAW, nl_listener->socket_protocol)) == -1) {
		perror("couldn't open netlink socket");
		return 0;
	}
	
	memset(&addr, 0, sizeof(addr));
	addr.nl_family = nl_listener->nl_family;
	addr.nl_groups = nl_listener->nl_groups;
	
// 	int flags;
// 	flags = fcntl(nl_listener->sockfd, F_GETFL, 0);
// 	if (flags == -1)
// 		flags = 0;
// 	fcntl(nl_listener->sockfd, F_SETFL, flags | O_NONBLOCK);
	
	if (bind(nl_listener->sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("couldn't bind");
		return 0;
	}
	
// 	nl_listener->base.thread = get_thread(netlink_raw_tmain, nl_listener, 0);
// 	nl_listener->base.thread->do_stop = &stop_thread;
	
// 	nl_listener->base.evloop_fd.fd = nl_listener->sockfd;
// 	nl_listener->base.evloop_fd.crtx_event_flags = EVLOOP_READ;
// 	nl_listener->base.evloop_fd.data = nl_listener;
// 	nl_listener->base.evloop_fd.event_handler = &netlink_el_event_handler;
// 	nl_listener->base.evloop_fd.event_handler_name = "netlink eventloop handler";
	crtx_evloop_init_listener(&nl_listener->base,
						nl_listener->sockfd,
						EVLOOP_READ,
						0,
						&netlink_el_event_handler,
						nl_listener,
						0,
						0
					);
	
	return &nl_listener->base;
}

void crtx_netlink_raw_init() {
}

void crtx_netlink_raw_finish() {
}
