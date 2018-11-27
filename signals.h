#ifndef _CRTX_SIGNALS_H
#define _CRTX_SIGNALS_H

#include "core.h"

struct crtx_signal_listener {
	struct crtx_listener_base base;
	int fd;
	
	int *signals; // { SIGINT, ..., 0 }
	char no_signalfd;
};

int crtx_signals_handle_std_signals(struct crtx_signal_listener **signal_lstnr);

struct crtx_listener_base *crtx_new_signals_listener(void *options);
void crtx_signals_init();
void crtx_signals_finish();

#endif
