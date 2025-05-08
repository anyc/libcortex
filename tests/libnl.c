/*
 * This example shows information about the link status of the network interfaces
 * of the system.
 *
 * Mario Kicherer (dev@kicherer.org) 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/netlink.h>
#include <netlink/genl/genl.h> // for struct nlmsghdr

#include "core.h"
#include "nl_route_raw.h" // for crtx_nl_route_raw2dict_interface
#include "libnl.h"

struct crtx_libnl_listener libnl;

static int libnl_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_libnl_listener *libnl;
	int r;
	
	libnl = (struct crtx_libnl_listener *) event->origin;
	
	// if this listener was just started, send a query to get the current link status
	if (event->data.type == 'u' && event->data.uint32 == CRTX_LSTNR_STARTED) {
		struct rtgenmsg rt_hdr = { .rtgen_family = AF_PACKET, };
		
		printf("listener started, sending initial request\n");
		
		r = nl_send_simple(libnl->sock, RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP, &rt_hdr, sizeof(rt_hdr));
		if (r < 0) {
			CRTX_ERROR("nl_send_simple failed: %d\n", r);
			return 1;
		}
	}
	
	return 0;
}

// called after received a netlink message, converts the message into a libcortex
// dict and prints the dict
int crtx_libnl_msg2dict(struct nl_msg *msg, void *arg) {
	struct crtx_dict *dict;
	
	printf("libnl message received\n");
	
	dict = crtx_nl_route_raw2dict_interface(nlmsg_hdr(msg), 1);
	
	crtx_print_dict(dict);
	
	crtx_dict_unref(dict);
	
	return NL_OK;
}

// called when all netlink messages were received for the initial query
int crtx_libnl_msg_end(struct nl_msg *msg, void *arg) {
	printf("all messages received for initial query\n");
	
	// either shutdown after the initial query or register to receive messages on changes
	#ifdef QUERY_ONLY
	crtx_init_shutdown();
	#else
	struct crtx_libnl_listener *libnl_lstnr;
	
	libnl_lstnr = (struct crtx_libnl_listener*) arg;
	
	printf("will wait on further events...\n");
	
	nl_socket_add_memberships(libnl_lstnr->sock, RTNLGRP_LINK, 0);
	#endif
	
	return NL_OK;
}

// structure for registering callback functions
struct crtx_libnl_callback libnl_callbacks[] = {
	{ NL_CB_VALID, NL_CB_CUSTOM, crtx_libnl_msg2dict, 0},
	{ NL_CB_FINISH, NL_CB_CUSTOM, crtx_libnl_msg_end, 0},
	{ 0 },
};

int main(int argc, char **argv) {
	struct crtx_libnl_callback *cbit;
	int r;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	memset(&libnl, 0, sizeof(struct crtx_libnl_listener));
	
	// set userdata for the callback functions
	for (cbit = libnl_callbacks; cbit->func; cbit++) {
		cbit->arg = &libnl;
	}
	libnl.callbacks = libnl_callbacks;
	
	r = crtx_setup_listener("libnl", &libnl);
	if (r) {
		CRTX_ERROR("create_listener(libnl) failed: %s\n", strerror(-r));
		exit(1);
	}
	
	// also send listener state changes as events to the listener's default graph
	libnl.base.state_graph = libnl.base.graph;
	// register a callback function that is called when events are received
	crtx_create_task(libnl.base.graph, 0, "libnl_test", libnl_test_handler, 0);
	
	r = crtx_start_listener(&libnl.base);
	if (r) {
		CRTX_ERROR("starting libnl listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	crtx_finish();
	
	return 0;
}
