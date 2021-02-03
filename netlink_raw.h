#ifndef _CRTX_NETLINK_RAW
#define _CRTX_NETLINK_RAW

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <sys/socket.h>
#include <linux/netlink.h>

#include "core.h"

struct crtx_netlink_raw_listener {
	struct crtx_listener_base base;
	
	int socket_protocol;
	
	sa_family_t nl_family;
	uint32_t nl_groups;
	
	int (*read_cb)(struct crtx_netlink_raw_listener *nl_listener, int fd, void *userdata);
	void *read_cb_userdata;
	
	int (*message_callback)(struct nlmsghdr *nlmsghdr, size_t nlmsgsize, void *message_callback_data);
	void *message_callback_data;
	uint16_t *message_types; // filter nlmsg types
	
	int fd;
	char stop;
};

struct crtx_listener_base *crtx_setup_netlink_raw_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(netlink_raw)

void crtx_netlink_raw_init();
void crtx_netlink_raw_finish();

#ifdef __cplusplus
}
#endif

#endif
