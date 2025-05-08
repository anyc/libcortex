/*
 * This example starts a timer. The timer forks the process once and writes
 * a message to stdout every second. After a few seconds, both processes are
 * terminated.
 *
 * Mario Kicherer (dev@kicherer.org) 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core.h"
#include "fork.h"
#include "timer.h"

struct crtx_fork_listener fork_lstnr;
struct crtx_timer_listener tlist;
struct crtx_listener_base *blist;

int forked = 0;
int counter = 0;

int start_timer();

// callback that is called after a fork by the child process
static void reinit_cb(void *reinit_cb_data) {
	printf("[%d] child reinit_cb\n", getpid());
	
	crtx_handle_std_signals();
	
	start_timer();
}

// callback that is executed if the child process terminates
static int fork_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	if (event->type == CRTX_FORK_ET_CHILD_STOPPED) {
		printf("[%d] child done with rc %d\n", getpid(), event->data.int32);
	} else
	if (event->type == CRTX_FORK_ET_CHILD_KILLED) {
		fprintf(stderr, "error, child killed with rc %d\n", event->data.int32);
	} else {
		fprintf(stderr, "unexpected event sent by fork listener %d\n", event->type);
	}
	
	return 0;
}

// timer function that is called every second
static int timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("[%d] received timer event: %u\n", getpid(), event->data.uint32);
	
	if (!forked) {
		int rv;
		
		forked = 1;
		
		fork_lstnr.reinit_cb = &reinit_cb;
		fork_lstnr.reinit_cb_data = 0;
		
		rv = crtx_setup_listener("fork", &fork_lstnr);
		if (rv) {
			CRTX_ERROR("create_listener(fork) failed\n");
			return -1;
		}
		
		if (!fork_lstnr.base.graph->tasks)
			crtx_create_task(fork_lstnr.base.graph, 0, "fork_event_handler", &fork_event_handler, 0);
		
		printf("[%d] forking...\n", getpid());
		
		rv = crtx_start_listener(&fork_lstnr.base);
		if (rv) {
			CRTX_ERROR("starting signal listener failed\n");
			return rv;
		}
	}
	
	counter += 1;
	if (fork_lstnr.pid == 0) {
		if (counter == 3)
			crtx_init_shutdown();
	} else {
		if (counter == 5)
			crtx_init_shutdown();
	}
	
	return 1;
}

// setup the timer
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
		printf("[%d] CHILD TERM\n", getpid());
	} else {
		printf("[%d] PARENT TERM\n", getpid());
	}
	
	crtx_finish();
	
	return rv;
}
