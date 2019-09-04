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

#include "intern.h"
#include "core.h"
#include "fork.h"
#include "signals.h"
#include "timer.h"

#ifndef CRTX_TEST

// static int sigchld_signal[] = {SIGCHLD, 0};

static void reinit_cb(void *reinit_cb_data) {
	struct crtx_fork_listener *flstnr;
	
	flstnr = (struct crtx_fork_listener *) reinit_cb_data;
	
	DBG("calling reinit_cb()\n");
	
	flstnr->reinit_cb(flstnr->reinit_cb_data);
}

static char do_fork(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_fork_listener *flstnr;
	
	
	flstnr = (struct crtx_fork_listener *) userdata;
	
	flstnr->pid = fork();
	
	if (flstnr->pid < 0) {
		fprintf(stderr, "fork failed: %s\n", strerror(errno));
		exit(1);
	} else 
	if (flstnr->pid == 0) {
		DBG("child shutting down\n");
		
		crtx_root->reinit_after_shutdown = 1;
		crtx_root->reinit_cb = &reinit_cb;
		crtx_root->reinit_cb_data = flstnr;
		crtx_shutdown_after_fork();
	} else
	if (flstnr->pid > 0) {
// 		struct crtx_event *event;
		
		DBG("fork done\n");
		
// 		crtx_create_event(&event);
// 		event.type = CRTX_FORK_EVT_FORK_DONE_PARENT;
// 		
// 		crtx_add_event(fork_lstnr.base.graph, event);
	}
	
	return 0;
}

static char start_listener(struct crtx_listener_base *lstnr) {
	struct crtx_fork_listener *flstnr;
// 	int ret;
	
	
	flstnr = (struct crtx_fork_listener *) lstnr;
	
	flstnr->base.mode = CRTX_NO_PROCESSING_MODE;
	
// 	ret = crtx_start_listener(&flstnr->signal_lstnr.base);
// 	if (ret) {
// 		ERROR("starting signal listener failed\n");
// 		return ret;
// 	}
	
	flstnr->base.default_el_cb.event_handler = &do_fork;
	flstnr->base.default_el_cb.event_handler_data = flstnr;
	
	flstnr->base.default_el_cb.crtx_event_flags = EVLOOP_SPECIAL;
	crtx_evloop_trigger_callback(crtx_root->event_loop, &flstnr->base.default_el_cb);
	
	return 0;
}

static char stop_listener(struct crtx_listener_base *listener) {
	int r;
	struct crtx_fork_listener *flstnr;
	
	
	flstnr = (struct crtx_fork_listener *) listener;
	
	printf("sendkill %d\n", flstnr->pid);
	
// 	r = kill(flstnr->pid, SIGTERM);
// 	if (r != 0) {
// 		ERROR("killing pid %d failed: %s\n", flstnr->pid, strerror(errno));
// 		return 1;
// 	}
	
	return 0;
}

struct crtx_listener_base *crtx_new_fork_listener(void *options) {
	struct crtx_fork_listener *lstnr;
// 	int rv;
	
	
	lstnr = (struct crtx_fork_listener *) options;
	
	lstnr->base.start_listener = &start_listener;
	lstnr->base.stop_listener = &stop_listener;
	
// 	lstnr->signal_lstnr.signals = sigchld_signal;
// 	lstnr->signal_lstnr.base.graph = lstnr->base.graph;
// 	
// 	rv = crtx_create_listener("signals", &lstnr->signal_lstnr);
// 	if (rv) {
// 		ERROR("create_listener(signal) failed\n");
// 		return 0;
// 	}
	
	crtx_signals_add_child_handler(lstnr->sigchld_cb, lstnr);
	
// 	crtx_create_task(lstnr->signal_lstnr.base.graph, 0, "sigchld_handler", &sigchild_event_handler, 0);
	
	return &lstnr->base;
}

void crtx_fork_init() {
}

void crtx_fork_finish() {
}

#else /* CRTX_TEST */

struct crtx_fork_listener fork_lstnr;
struct crtx_timer_listener tlist;
struct crtx_listener_base *blist;
int forked = 0;

void reinit_cb(void *reinit_cb_data) {
	printf("child reinit_cb\n");
	
	crtx_init_shutdown();
}

void fork_sigchld_cb(pid_t pid, int status, void *userdata) {
	printf("child %d result: %d\n", pid, status);
}

static char timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("[%d] received timer event: %u\n", getpid(), event->data.uint32);
	
	if (!forked) {
		int rv;
		
		forked = 1;
		
		fork_lstnr.reinit_cb = &reinit_cb;
		fork_lstnr.reinit_cb_data = 0;
		fork_lstnr.sigchld_cb = &fork_sigchld_cb;
		
		rv = crtx_create_listener("fork", &fork_lstnr);
		if (rv) {
			ERROR("create_listener(fork) failed\n");
			return -1;
		}
		
// 		crtx_create_task(fork_lstnr.base.graph, 0, "fork_event_handler", fork_event_handler, 0);
		
		rv = crtx_start_listener(&fork_lstnr.base);
		if (rv) {
			ERROR("starting signal listener failed\n");
			return rv;
		}
	}
	
	return 1;
}

int start_timer() {
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
	
	blist = create_listener("timer", &tlist);
	if (!blist) {
		ERROR("create_listener(timer) failed\n");
		exit(1);
	}
	
	crtx_create_task(blist->graph, 0, "timertest", timertest_handler, 0);
	
	crtx_start_listener(blist);
	
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