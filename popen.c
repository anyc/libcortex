/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "intern.h"
#include "core.h"
#include "popen.h"
#include "signals.h"

#include "timer.h"

#ifndef CRTX_TEST

static void local_sigchld_cb(pid_t pid, int status, void *userdata) {
	struct crtx_popen_listener *plstnr;
	
	plstnr = (struct crtx_popen_listener *) userdata;
	
	if (pid == plstnr->pid) {
		printf("sigchld\n");
	}
}

static char start_listener(struct crtx_listener_base *listener) {
	pid_t pid;
	struct crtx_popen_listener *plstnr;
	
	
	plstnr = (struct crtx_popen_listener *) listener;
	
	crtx_signals_add_child_handler(&local_sigchld_cb, listener);
	
	pid = fork();
	if (pid < 0 ) {
		printf("error\n");
	} else
	if ( pid == 0 ) {
		printf("child: %d parent: %d\n", getpid(), getppid());
		
		if (plstnr->cmd) {
			char *argv[3] = {"Command-line", NULL};
			
			execvp(plstnr->cmd, argv);
		}
	} else {
		printf("parent: %d child: %d\n", getpid(), pid);
		
		plstnr->pid = pid;
		
// 		waitpid(pid, NULL, 0);

// 		printf("child finihsed\n");
	}
	
	plstnr->base.mode = CRTX_NO_PROCESSING_MODE;
	
	return 0;
}

static char stop_listener(struct crtx_listener_base *listener) {
	int r;
	struct crtx_popen_listener *plstnr;
	
	
	plstnr = (struct crtx_popen_listener *) listener;
	printf("sendkill %d\n", plstnr->pid);
	r = kill(plstnr->pid, SIGTERM);
	if (r != 0) {
		ERROR("stop popen failed: %s\n", strerror(errno));
		return 1;
	}
	
	return 0;
}

struct crtx_listener_base *crtx_new_popen_listener(void *options) {
	struct crtx_popen_listener *plstnr;
	
	plstnr = (struct crtx_popen_listener *) options;
	
	plstnr->base.start_listener = &start_listener;
	plstnr->base.stop_listener = &stop_listener;
	
	return &plstnr->base;
}

void crtx_popen_init() {
}

void crtx_popen_finish() {
}


#else /* CRTX_TEST */

struct crtx_popen_listener plist;
static int std_signals[] = {SIGTERM, SIGINT, 0};

static char timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("timer %d\n", getpid());
	
	return 0;
}

static char signal_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct signal_map *s;
	
	s = crtx_get_signal_info(event->data.uint32);
	
	printf("signal %s\n", s->name);
	
	if (plist.pid > 0)
		crtx_stop_listener(&plist.base);
	else
		crtx_init_shutdown();
	
	return 0;
}


int popen_main(int argc, char **argv) {
// 	struct crtx_popen_listener clist;
	int ret;
// 	pid_t pid;
	struct crtx_timer_listener tlist;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	struct crtx_signal_listener slist;
	
	memset(&slist, 0, sizeof(struct crtx_signal_listener));
	memset(&plist, 0, sizeof(struct crtx_popen_listener));
	
	slist.signals = std_signals;
	
	ret = crtx_create_listener("signals", &slist);
	if (ret) {
		ERROR("create_listener(signal) failed\n");
		return 0;
	}
	
	crtx_create_task(slist.base.graph, 0, "signal_test", &signal_event_handler, 0);
	
	ret = crtx_start_listener(&slist.base);
	if (ret) {
		ERROR("starting signal listener failed\n");
		return 0;
	}
	
	
	
// 	plist.cmd = "whoami";
	
	ret = crtx_create_listener("popen", &plist);
	if (ret) {
		ERROR("create_listener(popen) failed\n");
		return 0;
	}
	
// 	crtx_create_task(plist.base.graph, 0, "signal_test", &sigchild_event_handler, 0);
	
	ret = crtx_start_listener(&plist.base);
	if (ret) {
		ERROR("starting popen listener failed\n");
		return 0;
	}

	
	{
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
		
		ret = crtx_create_listener("timer", &tlist);
		if (ret != 0) {
			ERROR("create_listener(timer) failed\n");
			exit(1);
		}
		
		crtx_create_task(tlist.base.graph, 0, "timertest", timertest_handler, 0);
		
		crtx_start_listener(&tlist.base);
	}
	
/*	
	pid = fork();
	if (pid < 0 ) {
		printf("error\n");
	} else
	if ( pid == 0 ) {
		printf("child: %d parent: %d\n", getpid(), getppid());
		
// 		char *argv[3] = {"Command-line", NULL};
		
// 		execvp( "whoami", argv);
	} else {
		printf("parent: %d child: %d\n", getpid(), pid);
		
// 		waitpid(pid, NULL, 0);
		
// 		printf("child finihsed\n");
	}*/
	
// 	memset(&clist, 0, sizeof(struct crtx_popen_listener));
	
	
// 	ret = crtx_create_listener("popen", &clist);
// 	if (ret) {
// 		ERROR("create_listener(popen) failed\n");
// 		return 0;
// 	}
// 	
// 	crtx_create_task(clist.base.graph, 0, "popen_test", &popen_event_handler, 0);
// 	
// 	ret = crtx_start_listener(&clist.base);
// 	if (ret) {
// 		ERROR("starting popen listener failed\n");
// 		return 0;
// 	}
	
	crtx_loop();
	
	return 0;
}

// CRTX_TEST_MAIN(popen_main);
int main(int argc, char **argv) {
	int i;
	crtx_init();
// 	crtx_handle_std_signals();
	i = popen_main(argc, argv);
	crtx_finish();
	return i;
}
#endif
