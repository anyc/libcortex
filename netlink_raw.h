#ifndef _CRTX_NETLINK_RAW
#define _CRTX_NETLINK_RAW

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <sys/socket.h>

#include "core.h"

struct crtx_netlink_raw_listener {
	struct crtx_listener_base parent;
	
	int socket_protocol;
	
	sa_family_t nl_family;
	uint32_t nl_groups;
	
// 	uint16_t *nlmsg_types;
	
// 	struct crtx_dict *(*raw2dict)(struct crtx_netlink_raw_listener *nl_listener, struct nlmsghdr *nlh);
// 	char all_fields;
	
	int (*read_cb)(struct crtx_netlink_raw_listener *nl_listener, int fd, void *userdata);
	void *read_cb_userdata;
	
// 	struct crtx_signal msg_done;
	
	int sockfd;
	char stop;
};

struct crtx_listener_base *crtx_new_netlink_raw_listener(void *options);
void crtx_netlink_raw_init();
void crtx_netlink_raw_finish();

#endif
