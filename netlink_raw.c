
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/netlink.h>

#include "core.h"
#include "netlink_raw.h"

static void *netlink_raw_tmain(void *data) {
	struct crtx_netlink_raw_listener *nl_listener;
	int r;
	
	nl_listener = (struct crtx_netlink_raw_listener*) data;
	
	r = 0;
	while (!nl_listener->stop && r == 0) {
		r = nl_listener->read_cb(nl_listener, nl_listener->sockfd, nl_listener->read_cb_userdata);
	}
	
	return 0;
}

static char netlink_el_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_netlink_raw_listener *nl_listener;
	struct crtx_event_loop_payload *payload;
	
	payload = (struct crtx_event_loop_payload*) event->data.raw.pointer;
	
	nl_listener = (struct crtx_netlink_raw_listener*) payload->data;
	
	nl_listener->read_cb(nl_listener, nl_listener->sockfd, nl_listener->read_cb_userdata);
	
	return 0;
}

static void stop_thread(struct crtx_thread *t, void *data) {
	struct crtx_netlink_raw_listener *nl_listener;
	
	DBG("stopping netlink_raw listener\n");
	
	nl_listener = (struct crtx_netlink_raw_listener*) data;
	
	nl_listener->stop = 1;
	
	if (nl_listener->sockfd)
		shutdown(nl_listener->sockfd, SHUT_RDWR);
	
// 	if (nl_listener->parent.thread)
// 		crtx_threads_interrupt_thread(nl_listener->parent.thread);
	
// 	wait_on_signal(&nl_listener->parent.thread->finished);
// 	dereference_signal(&nl_listener->parent.thread->finished);
}

// void shutdown_netlink_raw_listener(struct crtx_listener_base *lbase) {
// 	struct crtx_netlink_raw_listener *nl_listener;
// 	
// 	nl_listener = (struct crtx_netlink_raw_listener *) lbase;
// 	
// 	stop_thread(lbase->thread, nl_listener);
// 	
// // 	free_signal(&nl_listener->msg_done);
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
	
	if (bind(nl_listener->sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("couldn't bind");
		return 0;
	}
	
	new_eventgraph(&nl_listener->parent.graph, 0, 0);
	
// 	init_signal(&nl_listener->msg_done);
	
// 	nl_listener->parent.shutdown = &shutdown_netlink_raw_listener;
	
	nl_listener->parent.thread = get_thread(netlink_raw_tmain, nl_listener, 0);
	nl_listener->parent.thread->do_stop = &stop_thread;
	
	nl_listener->parent.el_payload.fd = nl_listener->sockfd;
	nl_listener->parent.el_payload.data = nl_listener;
	nl_listener->parent.el_payload.event_handler = &netlink_el_event_handler;
	nl_listener->parent.el_payload.event_handler_name = "netlink eventloop handler";
	
// 	reference_signal(&nl_listener->parent.thread->finished);
	
	return &nl_listener->parent;
}

void crtx_netlink_raw_init() {
}

void crtx_netlink_raw_finish() {
}
