
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crtx/core.h>
#include <crtx/cache.h>
#include <crtx/dict.h>
#include <crtx/nl_route_raw.h>

#include <net/if.h>

int ipq_get_interfaces_libc(unsigned int *n_interfaces) {
	struct if_nameindex *interfaces, *if_iter;
	
	interfaces = if_nameindex();
	if (!interfaces)
		return 0;
	
	for (if_iter = interfaces; if_iter->if_index != 0 || if_iter->if_name != NULL; if_iter++)
	{
		printf("%s\n", if_iter->if_name);
	}
	
	if_freenameindex(interfaces);
}

struct ipq_monitor {
	struct crtx_nl_route_raw_listener *nlr_list;
	
	struct crtx_task *cache_task;
	
	struct crtx_signals *initial_response;
};

static void ipq_trigger_signal(void *data) {
	struct crtx_signals *signal;
	
	signal = (struct crtx_signals *) data;
	crtx_send_signal(signal, 0);
}

static char ipq_get_own_ip_addresses_keygen(struct crtx_event *event, struct crtx_dict_item *key) {
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


void ipq_free_monitor(struct ipq_monitor *monitor) {
	crtx_shutdown_signal(monitor->initial_response);
	
	if (monitor->nlr_list)
		crtx_shutdown_listener(&monitor->nlr_list->base);
	
	if (monitor->cache_task)
		crtx_free_response_cache_task(monitor->cache_task);
	
	free(monitor);
}

struct {
	struct nlmsghdr n;
	struct ifaddrmsg r;
} addr_req;

static uint16_t ipq_get_ips_nlmsg_addr_types[] = {RTM_NEWADDR, RTM_DELADDR, 0};

int ipq_get_ips_setup(struct ipq_monitor **monitor_p) {
	struct ipq_monitor *monitor;
	struct crtx_nl_route_raw_listener *nlr_list;
	int rc;
	struct crtx_cache_task *ctask;
	
	
	monitor = (struct ipq_monitor *) calloc(1, sizeof(struct ipq_monitor));
	*monitor_p = monitor;
	
	monitor->nlr_list = crtx_nl_route_raw_calloc_listener();
	nlr_list = monitor->nlr_list;
	
	
	nlr_list->nl_listener.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
	nlr_list->nlmsg_types = ipq_get_ips_nlmsg_addr_types;
	
	nlr_list->raw2dict = &crtx_nl_route_raw2dict_ifaddr;
	nlr_list->all_fields = 1;
	
	monitor->initial_response = crtx_alloc_signal();
	crtx_init_signal(monitor->initial_response);
	
	nlr_list->msg_done_cb = &ipq_trigger_signal;
	nlr_list->msg_done_cb_data = monitor->initial_response;
	
	rc = crtx_setup_listener("nl_route_raw", nlr_list);
	if (rc) {
		CRTX_ERROR("create_listener(nl_route_raw) failed: %s\n", strerror(-rc));
		return rc;
	}
	
	{
		rc = crtx_create_response_cache_task(&monitor->cache_task, "ip_cache", ipq_get_own_ip_addresses_keygen);
		if (rc) {
			ipq_free_monitor(monitor);
			return rc;
		}
		
		ctask = (struct crtx_cache_task*) monitor->cache_task->userdata;
		
		ctask->on_hit = &crtx_cache_update_on_hit;
		ctask->on_miss = &crtx_cache_add_on_miss;
		
		crtx_add_task(nlr_list->base.graph, monitor->cache_task);
	}
	
	return 0;
}

int ipq_get_ips_start(struct ipq_monitor *monitor) {
	int rc;
	
	
	rc = crtx_start_listener(&monitor->nlr_list->base);
	if (rc) {
		CRTX_ERROR("starting nl_route_raw listener failed\n");
		return rc;
	}
	
	// initially, request a list of addresses
	memset(&addr_req, 0, sizeof(addr_req));
	addr_req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	addr_req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP; // NLM_F_ATOMIC
	addr_req.n.nlmsg_type = RTM_GETADDR;
// 	addr_req.r.ifa_family = AF_INET; // e.g., AF_INET (IPv4-only) or AF_INET6 (IPv6-only)
	
	crtx_reset_signal(monitor->initial_response);
	
	crtx_nl_route_send_req(monitor->nlr_list, &addr_req.n);
	
	return 0;
}

int ipq_get_ips_stop(struct ipq_monitor *monitor) {
	if (crtx_separate_evloop_thread()) {
		crtx_wait_on_signal(monitor->initial_response, 0);
	} else {
		while (!crtx_signal_is_active(monitor->initial_response)) {
			crtx_loop_onetime();
		}
	}
	
	crtx_stop_listener(&monitor->nlr_list->base);
	
	if (crtx_separate_evloop_thread()) {
		crtx_wait_on_graph_empty(monitor->nlr_list->base.graph);
	} else {
		while (!crtx_is_graph_empty(monitor->nlr_list->base.graph)) {
			crtx_loop_onetime();
		}
	}
	
	return 0;
}

int ipq_get_ips_finish(struct ipq_monitor *monitor) {
	ipq_free_monitor(monitor);
	
	return 0;
}

int ipq_get_ips(struct crtx_dll **ips) {
	int rc;
	struct ipq_monitor *monitor;
	
	crtx_start_detached_event_loop();
	
	rc = ipq_get_ips_setup(&monitor);
	if (rc) {
		ipq_get_ips_finish(monitor);
		return rc;
	}
	
	rc = ipq_get_ips_start(monitor);
	if (rc) {
		ipq_get_ips_finish(monitor);
		return rc;
	}
	
	rc = ipq_get_ips_stop(monitor);
	if (rc) {
		ipq_get_ips_finish(monitor);
		return rc;
	}
	
	crtx_print_dict(((struct crtx_cache_task*) monitor->cache_task->userdata)->cache->entries);
	
	rc = ipq_get_ips_finish(monitor);
	if (rc)
		return rc;
	
	return 0;
}

int ipq_init() {
	crtx_init();
}

int ipq_finish() {
	crtx_finish();
}
