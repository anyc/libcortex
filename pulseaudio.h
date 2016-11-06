
#ifndef _CRTX_PA_H
#define _CRTX_PA_H

#include "core.h"
#include "sd_bus.h"

#define CRTX_PA_MSG_ETYPE "pulseaudio/event"
extern char *crtx_pa_msg_etype[];

struct crtx_pa_listener {
	struct crtx_listener_base parent;
	
	struct crtx_sdbus_listener sdbus_list;
};

struct crtx_listener_base *crtx_new_pa_listener(void *options);

void crtx_pa_init();
void crtx_pa_finish();

#endif
