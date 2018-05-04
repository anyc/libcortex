#ifndef _CRTX_NETLINK_GE_H
#define _CRTX_NETLINK_GE_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

typedef void (*crtx_genl_parse_cb)(struct nlattr * attr, struct crtx_dict *dict);

struct crtx_genl_group {
	char *family;
	char *group;
	
	unsigned int gid;
	
	
	unsigned int n_attrs;
	struct nlattr **attrs;
	// 	char *attr_signature;
	struct crtx_dict *attrs_template;
	crtx_genl_parse_cb *parse_callbacks;
};

struct crtx_genl_callback {
	enum nl_cb_type type;
	enum nl_cb_kind kind;
	nl_recvmsg_msg_cb_t func;
	void * arg;
};

struct crtx_genl_listener {
	struct crtx_listener_base parent;
	
	struct nl_sock *sock;
	
	struct crtx_genl_callback *callbacks;
	struct crtx_genl_group *groups;
};

struct crtx_listener_base *crtx_new_genl_listener(void *options);
// char crtx_netlink_raw_send_req(struct crtx_netlink_listener *nl_listener, struct nlmsghdr *n);
void crtx_netlink_ge_init();
void crtx_netlink_ge_finish();

#endif
