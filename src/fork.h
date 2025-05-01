#ifndef CRTX_FORK_H
#define CRTX_FORK_H

#ifdef __cplusplus
extern "C" {
#endif

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
	
	void (*pre_fork_callback)(void *cb_data);
	void *pre_fork_cb_data;
	
	void (*post_fork_child_callback)(void *cb_data);
	void *post_fork_child_cb_data;
	void (*post_fork_parent_callback)(void *cb_data);
	void *post_fork_parent_cb_data;
	
	void *sigchld_data;
	
	// TODO testing! If we do this, we might need to prevent shutdown of certain
	// listeners in the child to avoid changing the state of the parent process
	// as well.
	char reinit_immediately;
};

struct crtx_listener_base *crtx_setup_fork_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(fork)

void crtx_fork_init();
void crtx_fork_finish();

#ifdef __cplusplus
}
#endif

#endif
