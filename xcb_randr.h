#ifndef _CRTX_XCB_RANDR_H
#define _CRTX_XCB_RANDR_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <xcb/randr.h>

#include "core.h"

// #define XCB_RANDR_MSG_ETYPE "xcb/randr"
// extern char *xcb_randr_msg_etype[];

struct crtx_xcb_randr_listener {
	struct crtx_listener_base base;
	
	xcb_connection_t *conn;
	xcb_screen_t *screen;
};

struct crtx_listener_base *crtx_setup_xcb_randr_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(xcb_randr)

void crtx_xcb_randr_init();
void crtx_xcb_randr_finish();

#endif
