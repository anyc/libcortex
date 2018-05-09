/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/netlink.h>
#include <netlink/genl/genl.h> // for struct nlmsghdr

#include "intern.h"
#include "core.h"
#include "netif.h"
#include "nl_route_raw.h" // for crtx_nl_route_raw2dict_interface

int crtx_netif_query_interfaces(struct crtx_netif_listener *netif_lstnr) {
	int r;
	struct rtgenmsg rt_hdr = { .rtgen_family = AF_PACKET, };
	
	r = nl_send_simple(netif_lstnr->libnl_lstnr.sock, RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP, &rt_hdr, sizeof(rt_hdr));
	if (r < 0) {
		ERROR("nl_send_simple failed: %d\n", r);
		return 1;
	}
	
	return 0;
}

static char netif_state_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_netif_listener *netif_lstnr;
// 	struct crtx_libnl_listener *libnl;
	int r;
	
	netif_lstnr = (struct crtx_netif_listener *) userdata;
// 	libnl = (struct crtx_libnl_listener *) event->origin;
	
	if (event->data.type == 'u' && event->data.uint32 == CRTX_LSTNR_STARTED) {
		nl_socket_add_memberships(netif_lstnr->libnl_lstnr.sock, RTNLGRP_LINK, 0);
		
		if (netif_lstnr->query_existing > 0) {
			r = crtx_netif_query_interfaces(netif_lstnr);
			if (r)
				return r;
		}
	}
	
	return 0;
}

int crtx_libnl_msg2dict(struct nl_msg *msg, void *arg) {
	struct crtx_netif_listener *netif_lstnr;
	struct crtx_dict *dict;
	struct crtx_event *event;
	
	
	netif_lstnr = (struct crtx_netif_listener*) arg;
	
	dict = crtx_nl_route_raw2dict_interface(nlmsg_hdr(msg), 1);
	
	event = crtx_create_event(0);
	
	crtx_event_set_dict_data(event, dict, 0);
	
	crtx_add_event(netif_lstnr->parent.graph, event);
	
	return NL_OK;
}

int crtx_libnl_msg_end(struct nl_msg *msg, void *arg) {
	struct crtx_netif_listener *netif_lstnr;
	
	netif_lstnr = (struct crtx_netif_listener*) arg;
	
	if (netif_lstnr->query_existing == 2) {
		crtx_stop_listener(&netif_lstnr->parent);
	}
	
	return NL_OK;
}

struct crtx_libnl_callback libnl_callbacks[] = {
	{ NL_CB_VALID, NL_CB_CUSTOM, crtx_libnl_msg2dict, 0},
	{ NL_CB_FINISH, NL_CB_CUSTOM, crtx_libnl_msg_end, 0},
	{ 0 },
};

static char netif_start_listener(struct crtx_listener_base *listener) {
	struct crtx_netif_listener *netif_lstnr;
	int ret;
	
	netif_lstnr = (struct crtx_netif_listener*) listener;
	
	ret = crtx_start_listener(&netif_lstnr->libnl_lstnr.parent);
	if (ret) {
		ERROR("starting libnl listener failed\n");
		return ret;
	}
	
	return 0;
}

static char netif_stop_listener(struct crtx_listener_base *listener) {
	struct crtx_netif_listener *netif_lstnr;
	
	netif_lstnr = (struct crtx_netif_listener*) listener;
	
	crtx_stop_listener(&netif_lstnr->libnl_lstnr.parent);
	
	return 0;
}

static void netif_shutdown_listener(struct crtx_listener_base *listener) {
	struct crtx_netif_listener *netif_lstnr;
	
	netif_lstnr = (struct crtx_netif_listener*) listener;
	
	crtx_free_listener(&netif_lstnr->libnl_lstnr.parent);
	netif_lstnr->libnl_lstnr.parent.graph = 0;
}

struct crtx_listener_base *crtx_new_netif_listener(void *options) {
	struct crtx_netif_listener *netif_lstnr;
	struct crtx_listener_base *lbase;
	struct crtx_libnl_callback *cbit;
	int ret;
	
	
	netif_lstnr = (struct crtx_netif_listener*) options;
	
	for (cbit = libnl_callbacks; cbit->func; cbit++) {
		cbit->arg = netif_lstnr;
	}
	netif_lstnr->libnl_lstnr.callbacks = libnl_callbacks;
	
// 	lbase = create_listener("nl_libnl", &netif_lstnr->libnl_lstnr);
	ret = crtx_create_listener(&lbase, "nl_libnl", &netif_lstnr->libnl_lstnr);
	if (ret) {
		ERROR("create_listener(libnl) failed\n");
		exit(1);
	}
	
	lbase->state_graph = lbase->graph;
	crtx_create_task(lbase->graph, 0, "netif_state_handler", netif_state_handler, netif_lstnr);
	
	netif_lstnr->parent.shutdown = &netif_shutdown_listener;
	netif_lstnr->parent.start_listener = &netif_start_listener;
	netif_lstnr->parent.stop_listener = &netif_stop_listener;
	
	return &netif_lstnr->parent;
}

void crtx_netif_init() {}
void crtx_netif_finish() {}


#ifdef CRTX_TEST

static char netif_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;
	
	if (event->data.type == 'u' && event->data.uint32 == CRTX_LSTNR_STOPPED) {
		crtx_init_shutdown();
	} else {
		crtx_event_get_payload(event, 0, 0, &dict);
		
		crtx_print_dict(dict);
	}
	
	return 0;
}

int netif_main(int argc, char **argv) {
	struct crtx_netif_listener netif_lstnr;
	struct crtx_listener_base *lbase;
	int r;
	
	
	memset(&netif_lstnr, 0, sizeof(struct crtx_netif_listener));
	
	netif_lstnr.query_existing = 2;
	
// 	lbase = create_listener("netif", &netif_lstnr);
	r = crtx_create_listener(&lbase, "netif", &netif_lstnr);
	if (r) {
		ERROR("create_listener(netif) failed\n");
		exit(1);
	}
	
	// we also want to receive status events of the listener in this graph
	lbase->state_graph = lbase->graph;
	
	crtx_create_task(lbase->graph, 0, "netif_event_handler", netif_event_handler, 0);
	
	r = crtx_start_listener(lbase);
	if (!r) {
		ERROR("starting netif listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	crtx_free_listener(&netif_lstnr.parent);
	
	return 0;
}

CRTX_TEST_MAIN(netif_main);

#endif
