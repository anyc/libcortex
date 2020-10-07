#ifndef _CRTX_PULSEAUDIO_H
#define _CRTX_PULSEAUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include "core.h"
#include "sdbus.h"

#include "pulse/pulseaudio.h"

#define CRTX_PA_MSG_ETYPE "pulseaudio/event"
extern char *crtx_pa_msg_etype[];

struct crtx_pa_listener {
	struct crtx_listener_base base;
	
	pa_proplist *client_proplist;
	enum pa_subscription_mask subscription_mask;
	
	pa_context *context;
	pa_mainloop *mainloop;
	enum pa_context_state state;
};

struct crtx_listener_base *crtx_setup_pa_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(pa)

void crtx_pa_init();
void crtx_pa_finish();

#ifdef __cplusplus
}
#endif

#endif
