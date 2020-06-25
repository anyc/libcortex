#ifndef _CRTX_EVDEV_H
#define _CRTX_EVDEV_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include "libevdev/libevdev.h"

// #define EVDEV_MSG_ETYPE "evdev/event"
// extern char *evdev_msg_etype[];

struct crtx_evdev_listener {
	struct crtx_listener_base base;
	
	char *device_path;
	
	char stop;
	int fd;
	struct libevdev *device;
};

struct crtx_listener_base *crtx_setup_evdev_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(evdev)

void crtx_evdev_init();
void crtx_evdev_finish();

#endif
