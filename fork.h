#ifndef CRTX_FORK_H
#define CRTX_FORK_H

/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include "core.h"
#include "signals.h"

#define CRTX_FORK_EVT_FORK_DONE_PARENT (1)

struct crtx_fork_listener {
	struct crtx_listener_base base;
	
// 	struct crtx_signal_listener signal_lstnr;
// 	sigchld_cb sigchld_cb;
	
	pid_t pid;
	
	void (*reinit_cb)(void *cb_data);
	void *reinit_cb_data;
};

struct crtx_listener_base *crtx_new_fork_listener(void *options);

void crtx_fork_init();
void crtx_fork_finish();

#endif
