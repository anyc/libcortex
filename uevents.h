
#ifndef _CRTX_UEVENTS_H
#define _CRTX_UEVENTS_H

#include "netlink_raw.h"

struct crtx_uevents_listener {
	struct crtx_listener_base parent;
	
	struct crtx_netlink_raw_listener nl_listener;
};

struct crtx_listener_base *crtx_new_uevents_listener(void *options);
void crtx_uevents_init();
void crtx_uevents_finish();

#endif