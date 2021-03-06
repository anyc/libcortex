#ifndef _CRTX_UDEV_H
#define _CRTX_UDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <libudev.h>

// #define UDEV_MSG_ETYPE "udev/event"
// extern char *udev_msg_etype[];

struct crtx_udev_raw2dict_attr_req {
	char *subsystem;
	char *device_type;
	
	char **attributes;
};

struct crtx_udev_listener {
	struct crtx_listener_base base;
	
	struct udev *udev;
	int fd;
	struct udev_monitor *monitor;
	
	char query_existing;
	char **sys_dev_filter;
};

struct crtx_dict *crtx_udev_raw2dict(struct udev_device *dev, struct crtx_udev_raw2dict_attr_req *r2ds, char make_persistent);

struct crtx_listener_base *crtx_setup_udev_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(udev)

void crtx_udev_init();
void crtx_udev_finish();

#ifdef __cplusplus
}
#endif

#endif
