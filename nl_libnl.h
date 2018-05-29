#ifndef _CRTX_NL_LIBNL_H
#define _CRTX_NL_LIBNL_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <netlink/netlink.h>

struct crtx_libnl_callback {
	enum nl_cb_type type;
	enum nl_cb_kind kind;
	nl_recvmsg_msg_cb_t func;
	void * arg;
};

struct crtx_libnl_listener {
	struct crtx_listener_base base;
	
	struct nl_sock *sock;
	
	struct crtx_libnl_callback *callbacks;
};

struct crtx_listener_base *crtx_new_libnl_listener(void *options);
void crtx_libnl_init();
void crtx_libnl_finish();

#endif
