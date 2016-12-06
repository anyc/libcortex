
#ifndef _CRTX_CAN_H
#define _CRTX_CAN_H


struct crtx_can_listener {
	struct crtx_listener_base parent;
	
	char *interface_name;
	int protocol;
	
	int sockfd;
	struct sockaddr_can addr;
};

struct crtx_listener_base *crtx_new_can_listener(void *options);

void crtx_can_init();
void crtx_can_finish();

#endif
