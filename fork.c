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

#ifndef CRTX_TEST

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

static char do_fork(struct crtx_event *event, void *userdata, void **sessiondata) {
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
	}
	
	return 0;
}

static void fork_sigchld_cb(pid_t pid, int status, void *userdata) {
	struct crtx_fork_listener *lstnr;
	
	
	lstnr = (struct crtx_fork_listener *) userdata;
	
	if (pid == lstnr->pid) {
		struct crtx_event *new_event;
		
		lstnr->pid = 0;
		
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
	
	return 0;
}

static char stop_listener(struct crtx_listener_base *listener) {
	int r;
	struct crtx_fork_listener *flstnr;
	
	
	flstnr = (struct crtx_fork_listener *) listener;
	
	if (flstnr->pid > 0) {
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

#else /* CRTX_TEST */

struct crtx_fork_listener fork_lstnr;
struct crtx_timer_listener tlist;
struct crtx_listener_base *blist;
int forked = 0;

int start_timer();

void reinit_cb(void *reinit_cb_data) {
	printf("child reinit_cb\n");
	
// 	crtx_init_shutdown();
	
	crtx_handle_std_signals();
	
	start_timer();
}

// void fork_sigchld_cb(pid_t pid, int status, void *userdata) {
// 	printf("child %d result: %d\n", pid, status);
// }

static char timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("[%d] received timer event: %u\n", getpid(), event->data.uint32);
	
	if (!forked) {
		int rv;
		
		forked = 1;
		
		fork_lstnr.reinit_cb = &reinit_cb;
		fork_lstnr.reinit_cb_data = 0;
// 		fork_lstnr.sigchld_cb = &fork_sigchld_cb;
		
		rv = crtx_setup_listener("fork", &fork_lstnr);
		if (rv) {
			CRTX_ERROR("create_listener(fork) failed\n");
			return -1;
		}
		
// 		crtx_create_task(fork_lstnr.base.graph, 0, "fork_event_handler", fork_event_handler, 0);
		
		rv = crtx_start_listener(&fork_lstnr.base);
		if (rv) {
			CRTX_ERROR("starting signal listener failed\n");
			return rv;
		}
	}
	
	return 1;
}

int start_timer() {
	int r;
	
	
	memset(&tlist, 0, sizeof(struct crtx_timer_listener));
	
	// set time for (first) alarm
	tlist.newtimer.it_value.tv_sec = 1;
	tlist.newtimer.it_value.tv_nsec = 0;
	
	// set interval for repeating alarm, set to 0 to disable repetition
	tlist.newtimer.it_interval.tv_sec = 1;
	tlist.newtimer.it_interval.tv_nsec = 0;
	
	tlist.clockid = CLOCK_REALTIME; // clock source, see: man clock_gettime()
	tlist.settime_flags = 0; // absolute (TFD_TIMER_ABSTIME), or relative (0) time, see: man timerfd_settime()
	// 	tlist.newtimer = &newtimer;
	
	r = crtx_setup_listener("timer", &tlist);
	if (r) {
		CRTX_ERROR("create_listener(timer) failed: %s\n", strerror(-r));
		exit(1);
	}
	
	crtx_create_task(tlist.base.graph, 0, "timertest", timertest_handler, 0);
	
	crtx_start_listener(&tlist.base);
	
	return 0;
}

int main(int argc, char **argv) {
	int rv;
	
	
	rv = 0;
	memset(&fork_lstnr, 0, sizeof(struct crtx_fork_listener));
	
	crtx_init();
	crtx_handle_std_signals();
	
	start_timer();
	
	crtx_loop();
	
	if (fork_lstnr.pid == 0) {
		printf("CHILD TERM\n");
	} else {
		printf("PARENT TERM\n");
	}
	
	crtx_finish();
	
	return rv;
}

#endif /* CRTX_TEST */
