
#ifndef _CRTX_AVAHI_H
#define _CRTX_AVAHI_H

#include "core.h"

struct crtx_avahi_listener {
	struct crtx_listener_base parent;
	
};

struct crtx_listener_base *crtx_new_avahi_listener(void *options);
void crtx_avahi_init();
void crtx_avahi_finish();

#endif
