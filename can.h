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
	struct crtx_listener_base parent;
	
	char *interface_name;
	char ignore_if_state;
	char setup_if_only;
	unsigned int bitrate;
	int protocol;
	
	struct nl_sock *socket;
	struct rtnl_link *link;
	int sockfd;
	struct sockaddr_can addr;
};

struct crtx_listener_base *crtx_new_can_listener(void *options);

void crtx_can_init();
void crtx_can_finish();

#endif
