
#ifndef _CRTX_NL_ROUTE_H
#define _CRTX_NL_ROUTE_H

#include <linux/netlink.h>

#include "netlink_raw.h"

struct crtx_nl_route_listener {
	struct crtx_listener_base parent;
	
	struct crtx_netlink_raw_listener nl_listener;
	
	uint16_t *nlmsg_types;
	
	struct crtx_dict *(*raw2dict)(struct crtx_nl_route_listener *nl_listener, struct nlmsghdr *nlh);
	char all_fields;
	
	struct crtx_signal msg_done;
};

struct crtx_listener_base *crtx_new_nl_route_listener(void *options);
char crtx_nl_route_send_req(struct crtx_nl_route_listener *nl_listener, struct nlmsghdr *n);
void crtx_nl_route_init();
void crtx_nl_route_finish();

#endif
