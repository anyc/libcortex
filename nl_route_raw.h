#ifndef _CRTX_NL_ROUTE_H
#define _CRTX_NL_ROUTE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "netlink_raw.h"

struct crtx_nl_route_raw_listener {
	struct crtx_listener_base base;
	
	struct crtx_netlink_raw_listener nl_listener;
	
	uint16_t *nlmsg_types;
	
	struct crtx_dict *(*raw2dict)(struct crtx_nl_route_raw_listener *nl_listener, struct nlmsghdr *nlh);
	char all_fields;
	
// 	struct crtx_signals msg_done;
	void (*msg_done_cb)();
	void *msg_done_cb_data;
};

struct crtx_dict *crtx_nl_route_raw2dict_ifaddr(struct crtx_nl_route_raw_listener *nlr_list, struct nlmsghdr *nlh);
struct crtx_dict *crtx_nl_route_raw2dict_ifaddr2(struct nlmsghdr *nlh, int all_fields);
struct crtx_dict *crtx_nl_route_raw2dict_interface(struct nlmsghdr *nlh, char all_fields);
char crtx_nl_route_send_req(struct crtx_nl_route_raw_listener *nl_listener, struct nlmsghdr *n);

struct crtx_listener_base *crtx_setup_nl_route_raw_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(nl_route_raw)
CRTX_DECLARE_CALLOC_FUNCTION(nl_route_raw)

void crtx_nl_route_raw_init();
void crtx_nl_route_raw_finish();

#ifdef __cplusplus
}
#endif

#endif
