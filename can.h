#ifndef _CRTX_CAN_H
#define _CRTX_CAN_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <linux/can.h>

// #define CAN_MSG_ETYPE "can/event"
// extern char *can_msg_etype[];

struct crtx_can_listener {
	struct crtx_listener_base base;
	
	char *interface_name;
	char ignore_if_state;
	char setup_if_only;
	char try_to_keep_if_up;
	char is_virtual_can;
	char discard_unused_data;
	unsigned int bitrate;
	int protocol;
	int send_buffer_size;
	int recv_buffer_size;
	
	struct nl_sock *socket;
	struct rtnl_link *link;
	int sockfd;
	struct sockaddr_can addr;
};

struct crtx_listener_base *crtx_setup_can_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(can)

void crtx_can_init();
void crtx_can_finish();

#endif
