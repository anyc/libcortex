#ifndef CRTX_FORK_H
#define CRTX_FORK_H

/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include "core.h"
#include "signals.h"

#define CRTX_EVENT_TYPE_FAMILY_FORK (600)
#define CRTX_FORK_ET_FORK_DONE_PARENT (CRTX_EVENT_TYPE_FAMILY_FORK + 1)
#define CRTX_FORK_ET_CHILD_STOPPED (CRTX_EVENT_TYPE_FAMILY_FORK + 2)
#define CRTX_FORK_ET_CHILD_KILLED (CRTX_EVENT_TYPE_FAMILY_FORK + 3)

struct crtx_fork_listener {
	struct crtx_listener_base base;
	
// 	struct crtx_signals_listener signal_lstnr;
// 	sigchld_cb sigchld_cb;
	
	pid_t pid;
	
	char *user;
	char *group;
	
	void (*reinit_cb)(void *cb_data);
	void *reinit_cb_data;
	
	void *sigchld_data;
};

struct crtx_listener_base *crtx_setup_fork_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(fork)

void crtx_fork_init();
void crtx_fork_finish();

#endif
