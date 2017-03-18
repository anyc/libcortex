
#ifndef _CRTX_UDEV_H
#define _CRTX_UDEV_H

#include <libudev.h>

// #define UDEV_MSG_ETYPE "udev/event"
// extern char *udev_msg_etype[];

struct crtx_udev_raw2dict_attr_req {
	char *subsystem;
	char *device_type;
	
	char **attributes;
};

struct crtx_udev_listener {
	struct crtx_listener_base parent;
	
	struct udev *udev;
	int fd;
	struct udev_monitor *monitor;
	
	char query_existing;
	char **sys_dev_filter;
};

struct crtx_listener_base *crtx_new_udev_listener(void *options);

void crtx_udev_init();
void crtx_udev_finish();

struct crtx_dict *crtx_udev_raw2dict(struct crtx_event *event, struct crtx_udev_raw2dict_attr_req *r2ds);

#endif
