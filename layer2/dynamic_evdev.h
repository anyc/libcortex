#ifndef _CRTX_DYNAMIC_EVDEV_H
#define _CRTX_DYNAMIC_EVDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <crtx/udev.h>
#include <crtx/evdev.h>

struct crtx_dynamic_evdev_listener {
	struct crtx_listener_base base;
	
	struct crtx_dll *evdev_listeners;
	struct crtx_udev_listener udev_lstnr;
	struct crtx_dll *(*alloc_new)();
	void (*on_free)(struct crtx_dll *dll);
	
	// this task will be added to each device-specific evdev listener
	crtx_handle_task_t handler;
	void *handler_userdata;
};

struct crtx_listener_base *crtx_setup_dynamic_evdev_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(dynamic_evdev)

void crtx_dynamic_evdev_init();
void crtx_dynamic_evdev_finish();

#ifdef __cplusplus
}
#endif

#endif
