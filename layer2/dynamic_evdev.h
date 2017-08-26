
#ifndef _CRTX_DYNAMIC_EVDEV_H
#define _CRTX_DYNAMIC_EVDEV_H

#include "udev.h"
#include "evdev.h"

struct crtx_dynamic_evdev_listener {
	struct crtx_listener_base parent;
	
	struct crtx_dll *evdev_listeners;
	struct crtx_udev_listener udev_lstnr;
	struct crtx_dll *(*alloc_new)();
	void (*on_free)(struct crtx_dll *dll);
	
	crtx_handle_task_t handler;
};

struct crtx_listener_base *crtx_new_dynamic_evdev_listener(void *options);

void crtx_dynamic_evdev_init();
void crtx_dynamic_evdev_finish();

#endif
