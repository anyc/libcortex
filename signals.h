
#include "core.h"

struct crtx_signal_listener {
	struct crtx_listener_base parent;
	
	int *signals; // { SIGINT, ..., 0 }
};

struct crtx_listener_base *crtx_new_signals_listener(void *options);
void crtx_signals_init();
void crtx_signals_finish();