#ifndef _CRTX_CAN_H
#define _CRTX_CAN_H

/*
 * Mario Kicherer (dev@kicherer.org) 2023
 *
 */

#include <linux/gpio.h>
#include <sys/ioctl.h>

// #define CAN_MSG_ETYPE "can/event"
// extern char *can_msg_etype[];

struct uio_device {
	char *name;
	unsigned int index;
};

#define CRTX_UIO_FLAG_NO_READ (1<<0)

struct crtx_uio_listener {
	struct crtx_listener_base base;
	
	char *name;
	struct uio_device *uio_device;
	
	int flags;
};

struct crtx_listener_base *crtx_setup_uio_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(uio)

void crtx_uio_init();
void crtx_uio_finish();

#endif
