#ifndef _CRTX_NL_ROUTE_H
#define _CRTX_NL_ROUTE_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "netlink_raw.h"

struct crtx_nl_route_listener {
	struct crtx_listener_base parent;
	
	struct crtx_netlink_raw_listener nl_listener;
	
	uint16_t *nlmsg_types;
	
	struct crtx_dict *(*raw2dict)(struct crtx_nl_route_listener *nl_listener, struct nlmsghdr *nlh);
	char all_fields;
	
// 	struct crtx_signal msg_done;
	void (*msg_done_cb)();
	void *msg_done_cb_data;
};

struct crtx_listener_base *crtx_new_nl_route_raw_listener(void *options);
char crtx_nl_route_send_req(struct crtx_nl_route_listener *nl_listener, struct nlmsghdr *n);
void crtx_nl_route_raw_init();
void crtx_nl_route_raw_finish();
struct crtx_dict *crtx_nl_route_raw2dict_ifaddr(struct crtx_nl_route_listener *nlr_list, struct nlmsghdr *nlh);
struct crtx_dict *crtx_nl_route_raw2dict_interface(struct nlmsghdr *nlh, char all_fields);
#endif
