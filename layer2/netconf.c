/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/netlink.h>
#include <netlink/genl/genl.h> // for struct nlmsghdr

#include "intern.h"
#include "core.h"
#include "netconf.h"
#include "nl_route_raw.h" // for crtx_nl_route_raw2dict_interface

#ifndef CRTX_TEST

int crtx_netconf_query_interfaces(struct crtx_netconf_listener *netconf_lstnr) {
	int r;
	struct rtgenmsg rt_hdr = { .rtgen_family = AF_PACKET, };
	
	if (
		((netconf_lstnr->monitor_types & CRTX_NETCONF_INTF) != 0) &&
		((netconf_lstnr->queried_types & CRTX_NETCONF_INTF) == 0)
		)
	{
		r = nl_send_simple(netconf_lstnr->libnl_lstnr.sock, RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP, &rt_hdr, sizeof(rt_hdr));
		if (r < 0) {
			ERROR("nl_send_simple failed: %d\n", r);
			return 1;
		}
		
		netconf_lstnr->queried_types |= CRTX_NETCONF_INTF;
		
		return 0;
	}
	
	if (
		((netconf_lstnr->monitor_types & CRTX_NETCONF_NEIGH) != 0) &&
		((netconf_lstnr->queried_types & CRTX_NETCONF_NEIGH) == 0)
	)
	{
		r = nl_send_simple(netconf_lstnr->libnl_lstnr.sock, RTM_GETNEIGH, NLM_F_REQUEST | NLM_F_DUMP, &rt_hdr, sizeof(rt_hdr));
		if (r < 0) {
			ERROR("nl_send_simple failed: %d\n", r);
			return 1;
		}
		
		netconf_lstnr->queried_types |= CRTX_NETCONF_NEIGH;
		
		return 0;
	}
	
	if (
		((netconf_lstnr->monitor_types & CRTX_NETCONF_IPV4_ADDR) != 0 || (netconf_lstnr->monitor_types & CRTX_NETCONF_IPV6_ADDR) != 0) &&
		((netconf_lstnr->queried_types & CRTX_NETCONF_IPV4_ADDR) == 0 && (netconf_lstnr->queried_types & CRTX_NETCONF_IPV6_ADDR) == 0)
		)
	{
		r = nl_send_simple(netconf_lstnr->libnl_lstnr.sock, RTM_GETADDR, NLM_F_REQUEST | NLM_F_DUMP, &rt_hdr, sizeof(rt_hdr));
		if (r < 0) {
			ERROR("nl_send_simple failed: %d\n", r);
			return 1;
		}
		
		netconf_lstnr->queried_types |= CRTX_NETCONF_IPV4_ADDR | CRTX_NETCONF_IPV6_ADDR;
		
		return 0;
	}
	
	return 0;
}

static char netconf_state_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_netconf_listener *netconf_lstnr;
	int r;
	
	netconf_lstnr = (struct crtx_netconf_listener *) userdata;
	
	if (event->data.type == 'u' && event->data.uint32 == CRTX_LSTNR_STARTED) {
		if ((netconf_lstnr->monitor_types & CRTX_NETCONF_INTF) != 0)
			nl_socket_add_membership(netconf_lstnr->libnl_lstnr.sock, RTNLGRP_LINK);
		if ((netconf_lstnr->monitor_types & CRTX_NETCONF_NEIGH) != 0)
			nl_socket_add_membership(netconf_lstnr->libnl_lstnr.sock, RTNLGRP_NEIGH);
		if ((netconf_lstnr->monitor_types & CRTX_NETCONF_IPV4_ADDR) != 0)
			nl_socket_add_membership(netconf_lstnr->libnl_lstnr.sock, RTNLGRP_IPV4_IFADDR);
		if ((netconf_lstnr->monitor_types & CRTX_NETCONF_IPV6_ADDR) != 0)
			nl_socket_add_membership(netconf_lstnr->libnl_lstnr.sock, RTNLGRP_IPV6_IFADDR);
		
		if (netconf_lstnr->query_existing > 0) {
			r = crtx_netconf_query_interfaces(netconf_lstnr);
			if (r)
				return r;
		}
	}
	
	return 0;
}

int crtx_libnl_msg2dict(struct nl_msg *msg, void *arg) {
	struct crtx_netconf_listener *netconf_lstnr;
	struct crtx_dict *dict;
	struct crtx_event *event;
	
	
	netconf_lstnr = (struct crtx_netconf_listener*) arg;
	
	dict = crtx_nl_route_raw2dict_ifaddr2(nlmsg_hdr(msg), 1);
	
	crtx_create_event(&event);
	crtx_event_set_dict_data(event, dict, 0);
	
	switch (nlmsg_hdr(msg)->nlmsg_type) {
		case RTM_NEWADDR:
		case RTM_DELADDR:
			event->type = CRTX_NETCONF_ET_IP_ADDR;
			break;
		case RTM_NEWLINK:
		case RTM_DELLINK:
			event->type = CRTX_NETCONF_ET_INTF;
			break;
		case RTM_NEWNEIGH:
		case RTM_DELNEIGH:
			event->type = CRTX_NETCONF_ET_NEIGH;
			break;
	}
	
	crtx_add_event(netconf_lstnr->base.graph, event);
	
	return NL_OK;
}

int crtx_libnl_msg_end(struct nl_msg *msg, void *arg) {
	struct crtx_netconf_listener *netconf_lstnr;
	int r;
	struct crtx_event *event;
	
	
	netconf_lstnr = (struct crtx_netconf_listener*) arg;
	
	crtx_create_event(&event);
	event->type = CRTX_NETCONF_ET_QUERY_DONE;
// 	event->data.type = 'i';
// 	event->data.int_t = netconf_lstnr->monitor_types & netconf_lstnr->queried_types) == netconf_lstnr->monitor_types;
	crtx_event_set_raw_data(event, 'i', 
		(netconf_lstnr->monitor_types & netconf_lstnr->queried_types) == netconf_lstnr->monitor_types,
		sizeof(int),
		0
		);
	crtx_add_event(netconf_lstnr->base.graph, event);
	
	if (netconf_lstnr->query_existing > 0) {
		if ((netconf_lstnr->monitor_types & netconf_lstnr->queried_types) != netconf_lstnr->monitor_types) {
			r = crtx_netconf_query_interfaces(netconf_lstnr);
			if (r)
				return r;
		} else {
			if (netconf_lstnr->query_existing == 2) {
				crtx_stop_listener(&netconf_lstnr->base);
			}
		}
	}
	
	return NL_OK;
}

struct crtx_libnl_callback libnl_callbacks[] = {
	{ NL_CB_VALID, NL_CB_CUSTOM, crtx_libnl_msg2dict, 0},
	{ NL_CB_FINISH, NL_CB_CUSTOM, crtx_libnl_msg_end, 0},
	{ 0 },
};

static char netconf_start_listener(struct crtx_listener_base *listener) {
	struct crtx_netconf_listener *netconf_lstnr;
	int ret;
	
	netconf_lstnr = (struct crtx_netconf_listener*) listener;
	
	ret = crtx_start_listener(&netconf_lstnr->libnl_lstnr.base);
	if (ret) {
		ERROR("starting libnl listener failed\n");
		return ret;
	}
	
	// we have no fd or thread to monitor
	netconf_lstnr->base.mode = CRTX_NO_PROCESSING_MODE;
	
	return 0;
}

static char netconf_stop_listener(struct crtx_listener_base *listener) {
	struct crtx_netconf_listener *netconf_lstnr;
	
	netconf_lstnr = (struct crtx_netconf_listener*) listener;
	
	crtx_stop_listener(&netconf_lstnr->libnl_lstnr.base);
	
	return 0;
}

static void netconf_shutdown_listener(struct crtx_listener_base *listener) {
	struct crtx_netconf_listener *netconf_lstnr;
	
	netconf_lstnr = (struct crtx_netconf_listener*) listener;
	
	crtx_free_listener(&netconf_lstnr->libnl_lstnr.base);
	netconf_lstnr->libnl_lstnr.base.graph = 0;
}

struct crtx_listener_base *crtx_new_netconf_listener(void *options) {
	struct crtx_netconf_listener *netconf_lstnr;
	// 	struct crtx_listener_base *lbase;
	struct crtx_libnl_callback *cbit;
	int ret;
	
	
	netconf_lstnr = (struct crtx_netconf_listener*) options;
	
	CRTX_STATIC_ASSERT(sizeof(libnl_callbacks) == sizeof(netconf_lstnr->libnl_callbacks), "size mismatch");
	memcpy(netconf_lstnr->libnl_callbacks, libnl_callbacks, sizeof(libnl_callbacks));
	
	for (cbit = netconf_lstnr->libnl_callbacks; cbit->func; cbit++) {
		cbit->arg = netconf_lstnr;
	}
	netconf_lstnr->libnl_lstnr.callbacks = netconf_lstnr->libnl_callbacks;
	
	// 	lbase = create_listener("nl_libnl", &netconf_lstnr->libnl_lstnr);
	ret = crtx_create_listener("nl_libnl", &netconf_lstnr->libnl_lstnr);
	if (ret) {
		ERROR("create_listener(libnl) failed\n");
		exit(1);
	}
	
	netconf_lstnr->libnl_lstnr.base.state_graph = netconf_lstnr->libnl_lstnr.base.graph;
	crtx_create_task(netconf_lstnr->libnl_lstnr.base.graph, 0, "netconf_state_handler", netconf_state_handler, netconf_lstnr);
	
	netconf_lstnr->base.shutdown = &netconf_shutdown_listener;
	netconf_lstnr->base.start_listener = &netconf_start_listener;
	netconf_lstnr->base.stop_listener = &netconf_stop_listener;
	
	return &netconf_lstnr->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(netconf)

void crtx_netconf_init() {}
void crtx_netconf_finish() {}





#else

static char netconf_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;
	
	if (event->data.type == 'u' && event->data.uint32 == CRTX_LSTNR_STOPPED) {
		crtx_init_shutdown();
	} else {
		crtx_event_get_payload(event, 0, 0, &dict);
		
		crtx_print_dict(dict);
	}
	
	return 0;
}

int netconf_main(int argc, char **argv) {
	struct crtx_netconf_listener netconf_lstnr;
	// 	struct crtx_listener_base *lbase;
	int r;
	
	
	memset(&netconf_lstnr, 0, sizeof(struct crtx_netconf_listener));
	
	netconf_lstnr.query_existing = 1;
	
	// 	lbase = create_listener("netconf", &netconf_lstnr);
	r = crtx_create_listener("netconf", &netconf_lstnr);
	if (r) {
		ERROR("create_listener(netconf) failed\n");
		exit(1);
	}
	
	// we also want to receive status events of the listener in this graph
	netconf_lstnr.base.state_graph = netconf_lstnr.base.graph;
	netconf_lstnr.monitor_types = CRTX_NETCONF_INTF | CRTX_NETCONF_NEIGH | CRTX_NETCONF_IPV4_ADDR | CRTX_NETCONF_IPV6_ADDR;
	
	crtx_create_task(netconf_lstnr.base.graph, 0, "netconf_event_handler", netconf_event_handler, 0);
	
	r = crtx_start_listener(&netconf_lstnr.base);
	if (r) {
		ERROR("starting netconf listener failed: %s (%d)\n", strerror(-r), r);
		return 1;
	}
	
	crtx_loop();
	
	crtx_free_listener(&netconf_lstnr.base);
	
	return 0;
}

CRTX_TEST_MAIN(netconf_main);

#endif
