
#ifndef _CRTX_CAN_H
#define _CRTX_CAN_H

#include <linux/can.h>

#define CAN_MSG_ETYPE "can/event"
extern char *can_msg_etype[];

struct crtx_can_listener {
	struct crtx_listener_base parent;
	
	char *interface_name;
	unsigned int bitrate;
	int protocol;
	
	int sockfd;
	struct sockaddr_can addr;
};

struct crtx_listener_base *crtx_new_can_listener(void *options);

void crtx_can_init();
void crtx_can_finish();

#endif
