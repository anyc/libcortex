/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>

#include "intern.h"
#include "core.h"
#include "timer.h"

pid_t my_pid = -2;

static char timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("[%d] received timer event: %u\n", getpid(), event->data.uint32);
	
	if (my_pid == -2) {
		my_pid = fork();
		if (my_pid < 0) {
			fprintf(stderr, "fork failed: %s\n", strerror(errno));
			exit(1);
		} else 
		if (my_pid == 0) {
			printf("child ready\n");
			
			crtx_shutdown_after_fork();
		} else
		if (my_pid > 0) {
		}
	}
	
	return 1;
}

int timer_main(int argc, char **argv) {
	struct crtx_timer_listener tlist;
	// 	struct itimerspec newtimer;
	struct crtx_listener_base *blist;
	
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
	
	crtx_loop();
	
	crtx_finish();
	
	return 0;
}

int main(int argc, char **argv) {
	int i;
	
	crtx_init();
	crtx_handle_std_signals();
	
	i = timer_main(argc, argv);
	
// 	crtx_finish();
	
	printf("APP END\n");
	
	char *s;
	asprintf(&s, "ls -lsh /proc/%d/fd/", getpid());
	system(s);
	free(s);
	
	if (my_pid == 0) {
		crtx_init();
		
		crtx_handle_std_signals();
		
		i = timer_main(argc, argv);
		
// 		crtx_finish();
		
		printf("CHILD TERM\n");
	} else {
		printf("PARENT TERM\n");
	}
	
	return i;
}
