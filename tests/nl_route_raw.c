/*
 * This example lists all IP addresses of the system.
 *
 * Mario Kicherer (dev@kicherer.org) 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core.h"
#include "nl_route_raw.h"
#include "cache.h"

// static uint16_t nlmsg_types[] = {RTM_NEWLINK, RTM_DELLINK, RTM_NEWADDR, RTM_DELADDR, 0};
static uint16_t nlmsg_addr_types[] = {RTM_NEWADDR, RTM_DELADDR, 0};

struct {
	struct nlmsghdr n;
	struct ifaddrmsg r;
} addr_req;

struct {
	struct nlmsghdr n;
	struct ifinfomsg ifinfomsg;
} if_req;

struct crtx_signals msg_done;

// called when all responses for the request were received
void msg_done_cb(void *data) {
	crtx_send_signal(&msg_done, 0);
}

// callback that creates a key for the cache entry from the received event
static int crtx_get_own_ip_addresses_keygen(struct crtx_cache *, struct crtx_dict_item *key, struct crtx_event *event) {
	struct crtx_dict_item *di;
	struct crtx_dict *dict;
	
	crtx_event_get_payload(event, 0, 0, &dict);
	
	di = crtx_get_item(dict, "address");
	
	key->key = 0;
	key->type = 's';
	key->string = crtx_stracpy(di->string, 0);
	key->flags |= CRTX_DIF_ALLOCATED_KEY;
	
	return 0;
}

char crtx_get_own_ip_addresses() {
	struct crtx_nl_route_raw_listener nlr_list;
	int ret;
	struct crtx_task *task;
	struct crtx_cache *cache;
	
	// we are interested in the addition and removal of IPv4 and v6 addresses
	memset(&nlr_list, 0, sizeof(struct crtx_nl_route_raw_listener));
	nlr_list.nl_listener.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
	nlr_list.nlmsg_types = nlmsg_addr_types;
	
	// register callback that converts a struct into a libcortex dict
	nlr_list.raw2dict = &crtx_nl_route_raw2dict_ifaddr;
	nlr_list.all_fields = 1;
	
	nlr_list.msg_done_cb = msg_done_cb;
	nlr_list.msg_done_cb_data = &nlr_list;
	
	ret = crtx_setup_listener("nl_route_raw", &nlr_list);
	if (ret) {
		CRTX_ERROR("create_listener(nl_route_raw) failed: %s\n", strerror(-ret));
		exit(1);
	}
	
	// setup cache task that will maintain a list of known IPs
	ret = crtx_create_cache_task(&task, "ip_cache", crtx_get_own_ip_addresses_keygen);
	if (ret) {
		CRTX_ERROR("crtx_create_cache_task() failed: %d\n", ret);
		return -1;
	}
	
	// use default functions that handle events
	cache = (struct crtx_cache *) task->userdata;
	cache->on_hit = &crtx_cache_update_on_hit;
	cache->on_miss = &crtx_cache_add_on_miss;
	
	crtx_add_task(nlr_list.base.graph, task);
	
	ret = crtx_start_listener(&nlr_list.base);
	if (ret) {
		CRTX_ERROR("starting nl_route_raw listener failed\n");
		return -1;
	}
	
	crtx_init_signal(&msg_done);
	crtx_reset_signal(&msg_done);
	
	// setup initial request to get a list of addresses
	memset(&addr_req, 0, sizeof(addr_req));
	addr_req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	addr_req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP; // NLM_F_ATOMIC
	addr_req.n.nlmsg_type = RTM_GETADDR;
// 	addr_req.r.ifa_family = AF_INET; // e.g., AF_INET (IPv4-only) or AF_INET6 (IPv6-only)
	
	// send request
	crtx_nl_route_send_req(&nlr_list, &addr_req.n);
	
	// wait until all messages were received
	crtx_wait_on_signal(&msg_done, 0);
	
	/*
	 * shutdown and release
	 */
	
	crtx_stop_listener(&nlr_list.base);
	crtx_wait_on_graph_empty(nlr_list.base.graph);
	
	crtx_print_dict(cache->entries);
	
	crtx_free_cache_task(task);
	
	crtx_shutdown_listener(&nlr_list.base);
	
	return 0;
}

int main(int argc, char **argv) {
	crtx_init();
	crtx_handle_std_signals();
	
	crtx_start_detached_event_loop();
	
	crtx_get_own_ip_addresses();
	
	crtx_finish();
	
	return 0;
}
