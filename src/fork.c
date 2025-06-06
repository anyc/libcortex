/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// #include <sys/types.h>
#include <sys/wait.h>

// getpwname
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include "intern.h"
#include "core.h"
#include "fork.h"
#include "signals.h"
#include "timer.h"

// static int sigchld_signal[] = {SIGCHLD, 0};

static void reinit_cb(void *reinit_cb_data) {
	struct crtx_fork_listener *flstnr;
	int rv;
	
	
	flstnr = (struct crtx_fork_listener *) reinit_cb_data;
	
	if (flstnr->group) {
		struct group grp;
		struct group *p_grp;
		char *buf;
		long bufsize;
		
		bufsize = sysconf(_SC_GETGR_R_SIZE_MAX);
		if (bufsize == -1)
			bufsize = 1024;
		
		buf = calloc(1, bufsize);
		
		rv = getgrnam_r(flstnr->group, &grp, buf, bufsize, &p_grp);
		if (rv != 0 || p_grp == 0) {
			CRTX_ERROR("getgrpnam_r(%s) failed: %s\n", flstnr->group, strerror(errno));
			exit(1);
		}
		
		CRTX_DBG("change group to %s (%d)\n", flstnr->group, grp.gr_gid);
		
		rv = setgid(grp.gr_gid);
		if (rv == -1) {
			CRTX_ERROR("setgid(%d) failed: %s\n", grp.gr_gid, strerror(errno));
			exit(1);
		}
		
		free(buf);
	}
	
	if (flstnr->user) {
		struct passwd pw;
		struct passwd *p_pw;
		char *buf;
		long bufsize;
		
		bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (bufsize == -1)
			bufsize = 1024;
		
		buf = calloc(1, bufsize);
		
		rv = getpwnam_r(flstnr->user, &pw, buf, bufsize, &p_pw);
		if (rv != 0 || p_pw == 0) {
			CRTX_ERROR("getpwnam_r(%s) failed: %s\n", flstnr->user, strerror(errno));
			exit(1);
		}
		
		CRTX_DBG("change user to %s (%d %d)\n", flstnr->user, pw.pw_uid, pw.pw_gid);
		
		rv = setuid(pw.pw_uid);
		if (rv == -1) {
			CRTX_ERROR("setuid(%d) failed: %s\n", pw.pw_uid, strerror(errno));
			exit(1);
		}
		
		if (!flstnr->group) {
			rv = setgid(pw.pw_gid);
			if (rv == -1) {
				CRTX_ERROR("setgid(%d) failed: %s\n", pw.pw_gid, strerror(errno));
				exit(1);
			}
		}
		
		free(buf);
	}
	
	CRTX_DBG("calling reinit_cb()\n");
	
	flstnr->reinit_cb(flstnr->reinit_cb_data);
}

static int do_fork(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_fork_listener *flstnr;
	
	
	flstnr = (struct crtx_fork_listener *) userdata;
	
	if (flstnr->pre_fork_callback)
		flstnr->pre_fork_callback(flstnr->pre_fork_cb_data);
	
	flstnr->pid = fork();
	
	if (flstnr->pid < 0) {
		CRTX_ERROR("fork failed: %s\n", strerror(errno));
		return -1;
	} else 
	if (flstnr->pid == 0) {
		CRTX_DBG("child (%d), will reinit after fork\n", getpid());
		
		// We must not send the terminate signal to the children of our parent,
		// but we still need to free the fork listeners. Hence, we set the
		// process id of the fork listeners to zero and they will be freed
		// later with the other listeners.
		{
			struct crtx_ll *lit;
			struct crtx_listener_base *lbase;
			struct crtx_fork_listener *fork_lstnr;
			
			for (lit = crtx_root->listeners; lit; lit=lit->next) {
				lbase = (struct crtx_listener_base*) lit->data;
				
				if (!strcmp(lbase->id, "fork")) {
					fork_lstnr = (struct crtx_fork_listener*) lbase;
					
					fork_lstnr->pid = 0;
				}
			}
		}
		
		if (flstnr->post_fork_child_callback)
			flstnr->post_fork_child_callback(flstnr->post_fork_child_cb_data);
		
		if (!flstnr->reinit_immediately) {
			crtx_root->reinit_after_shutdown = 1;
			crtx_root->reinit_cb = &reinit_cb;
			crtx_root->reinit_cb_data = flstnr;
			crtx_shutdown_after_fork();
		} else {
			// will probably not return as it will likely call exec*()
			reinit_cb(flstnr);
		}
	} else
	if (flstnr->pid > 0) {
// 		struct crtx_event *event;
		
		CRTX_DBG("fork done - parent continues, child: %d\n", flstnr->pid);
		
// 		crtx_create_event(&event);
// 		event.type = CRTX_FORK_ET_FORK_DONE_PARENT;
// 		
// 		crtx_add_event(fork_lstnr.base.graph, event);
		
		if (flstnr->post_fork_parent_callback)
			flstnr->post_fork_parent_callback(flstnr->post_fork_parent_cb_data);
	}
	
	return 0;
}

static void fork_sigchld_cb(pid_t pid, int status, void *userdata) {
	struct crtx_fork_listener *lstnr;
	
	
	lstnr = (struct crtx_fork_listener *) userdata;
	
	if (pid == lstnr->pid) {
		struct crtx_event *new_event;
		
		// we do not clear this so the parent process can still use this to
		// determine if it is the parent process after the child stopped.
		// lstnr->pid = 0;
		
		lstnr->flags |= CRTX_FORK_CHILD_STOPPED;
		
		crtx_stop_listener(&lstnr->base);
		
		crtx_create_event(&new_event);
		
		if (WIFEXITED(status)) {
			new_event->type = CRTX_FORK_ET_CHILD_STOPPED;
			crtx_event_set_raw_data(new_event, 'i', WEXITSTATUS(status), sizeof(status), 0);
		} else
		if (WIFSIGNALED(status)) {
			new_event->type = CRTX_FORK_ET_CHILD_KILLED;
			crtx_event_set_raw_data(new_event, 'i', WTERMSIG(status), sizeof(status), 0);
		}
		
		crtx_add_event(lstnr->base.graph, new_event);
	} else {
		// if the process receives a signal from a child, all signal handlers
		// are notified and the callbacks have to determine, if it is for them
	}
}

static char start_listener(struct crtx_listener_base *lstnr) {
	struct crtx_fork_listener *flstnr;
// 	int ret;
	
	
	flstnr = (struct crtx_fork_listener *) lstnr;
	
	flstnr->base.mode = CRTX_NO_PROCESSING_MODE;
	
// 	ret = crtx_start_listener(&flstnr->signal_lstnr.base);
// 	if (ret) {
// 		CRTX_ERROR("starting signal listener failed\n");
// 		return ret;
// 	}
	
	flstnr->sigchld_data = crtx_signals_add_child_handler(&fork_sigchld_cb, flstnr);
	
	flstnr->base.default_el_cb.event_handler = &do_fork;
	flstnr->base.default_el_cb.event_handler_data = flstnr;
	
	flstnr->base.default_el_cb.crtx_event_flags = CRTX_EVLOOP_SPECIAL;
	crtx_evloop_trigger_callback(crtx_root->event_loop, &flstnr->base.default_el_cb);
	
	flstnr->pid = 0;
	flstnr->flags &= ~CRTX_FORK_CHILD_STOPPED;
	
	return 0;
}

static char stop_listener(struct crtx_listener_base *listener) {
	int r;
	struct crtx_fork_listener *flstnr;
	
	
	flstnr = (struct crtx_fork_listener *) listener;
	
	if (flstnr->pid > 0 && (flstnr->flags & CRTX_FORK_CHILD_STOPPED) == 0) {
		CRTX_DBG("sending SIGTERM to pid %d\n", flstnr->pid);

		r = kill(flstnr->pid, SIGTERM);
		if (r != 0) {
			CRTX_ERROR("kill() pid %d failed: %s\n", flstnr->pid, strerror(errno));
			return 1;
		}
	} else {
		CRTX_VDBG("fork lstnr has no pid set\n");
	}
	
	crtx_signals_rem_child_handler(flstnr->sigchld_data);
	
	return 0;
}

struct crtx_listener_base *crtx_setup_fork_listener(void *options) {
	struct crtx_fork_listener *lstnr;
// 	int rv;
	
	
	lstnr = (struct crtx_fork_listener *) options;
	
	lstnr->base.start_listener = &start_listener;
	lstnr->base.stop_listener = &stop_listener;
	
// 	lstnr->signal_lstnr.signals = sigchld_signal;
// 	lstnr->signal_lstnr.base.graph = lstnr->base.graph;
// 	
// 	rv = crtx_setup_listener("signals", &lstnr->signal_lstnr);
// 	if (rv) {
// 		CRTX_ERROR("create_listener(signal) failed\n");
// 		return 0;
// 	}
	
// 	crtx_create_task(lstnr->signal_lstnr.base.graph, 0, "sigchld_handler", &sigchild_event_handler, 0);
	
	return &lstnr->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(fork)

void crtx_fork_init() {
}

void crtx_fork_finish() {
}
