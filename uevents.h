#ifndef _CRTX_UEVENTS_H
#define _CRTX_UEVENTS_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include "netlink_raw.h"

struct crtx_uevents_listener {
	struct crtx_listener_base base;
	
	struct crtx_netlink_raw_listener nl_listener;
};

struct crtx_dict *crtx_uevents_raw2dict(struct crtx_event *event);
struct crtx_listener_base *crtx_new_uevents_listener(void *options);
void crtx_uevents_init();
void crtx_uevents_finish();

#endif
