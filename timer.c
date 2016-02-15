
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>

#include "core.h"
#include "timer.h"

#define CRTX_EVT_TIMER "cortex.timer"

static void *timer_tmain(void *data) {
	struct crtx_timer_listener *tlist;
	uint64_t exp;
	ssize_t s;
	struct crtx_event *event;
	
	tlist = (struct crtx_timer_listener*) data;
	
	while (!tlist->stop) {
		s = read(tlist->fd, &exp, sizeof(uint64_t));
		if (s == -1 && errno == EINTR) {
			DBG("timerfd thread interrupted\n");
			break;
		}
		if (s != sizeof(uint64_t)) {
			ERROR("reading from timerfd failed: %zd != %" PRIu64 " %s (%d)\n", s, exp, strerror(errno), errno);
			break;
		}
		
		event = create_event(CRTX_EVT_TIMER, 0, 0);
		event->data.raw = (void*)(uintptr_t) exp;
		event->data.flags = CRTX_EVF_DONT_FREE_RAW;
		
		add_event(tlist->parent.graph, event);
	}
	
	return 0;
}

void crtx_free_timer_listener(struct crtx_listener_base *data) {
	
}

static void stop_thread(struct crtx_thread *thread, void *data) {
	struct crtx_timer_listener *tlist;
	
	tlist = (struct crtx_timer_listener*) data;
	
	tlist->stop = 1;
	close(tlist->fd);
}

struct crtx_listener_base *crtx_new_timer_listener(void *options) {
	struct crtx_timer_listener *tlist;
	int ret;
	struct crtx_thread *t;
	
	tlist = (struct crtx_timer_listener *) options;
	
	if (!tlist->newtimer)
		return 0;
	
	new_eventgraph(&tlist->parent.graph, 0, 0);
	
	tlist->fd = timerfd_create(tlist->clockid, 0);
	if (tlist->fd == -1) {
		ERROR("timerfd_create failed: %s\n", strerror(errno));
		return 0;
	}
	
	ret = timerfd_settime(tlist->fd, tlist->settime_flags, tlist->newtimer, NULL);
	if (ret == -1) {
		ERROR("timerfd_settime failed: %s\n", strerror(errno));
		return 0;
	}
	
	t = get_thread(timer_tmain, tlist, 0);
	t->do_stop = &stop_thread;
	start_thread(t);
	
	tlist->parent.free = &crtx_free_timer_listener;
	
	return &tlist->parent;
}

void crtx_timer_init() {
}

void crtx_timer_finish() {
}

#ifdef CRTX_TEST
static void timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("received timer event: %lu\n", (uintptr_t) event->data.raw);
}

int timer_main(int argc, char **argv) {
	struct crtx_timer_listener tlist;
	struct itimerspec newtimer;
	struct crtx_listener_base *blist;
	
	newtimer.it_value.tv_sec = 1;
	newtimer.it_value.tv_nsec = 0;
	
	newtimer.it_interval.tv_sec = 1;
	newtimer.it_interval.tv_nsec = 0;
	
	tlist.clockid = CLOCK_REALTIME;
	tlist.settime_flags = TFD_TIMER_ABSTIME;
	tlist.newtimer = &newtimer;
	
	blist = create_listener("timer", &tlist);
	if (!blist) {
		ERROR("create_listener(timer) failed\n");
		exit(1);
	}
	
	crtx_create_task(blist->graph, 0, "timertest", timertest_handler, 0);
	
	crtx_loop();
}

CRTX_TEST_MAIN(timer_main);
#endif