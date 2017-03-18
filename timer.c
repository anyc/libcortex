
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>

#include "core.h"
#include "timer.h"


// static void stop_thread(struct crtx_thread *thread, void *data) {
// 	struct crtx_timer_listener *tlist;
// 	
// 	tlist = (struct crtx_timer_listener*) data;
// 	
// 	tlist->stop = 1;
// 	if (tlist->fd > 0) {
// 		close(tlist->fd);
// 		tlist->fd = 0;
// 	}
// }

// static void *timer_tmain(void *data) {
// 	struct crtx_timer_listener *tlist;
// 	uint64_t exp;
// 	ssize_t s;
// 	struct crtx_event *event;
// 	
// 	tlist = (struct crtx_timer_listener*) data;
// 	
// 	while (!tlist->stop) {
// 		s = read(tlist->fd, &exp, sizeof(uint64_t));
// 		if (s == -1 && errno == EINTR) {
// 			DBG("timerfd thread interrupted\n");
// 			break;
// 		}
// 		if (s != sizeof(uint64_t)) {
// 			ERROR("reading from timerfd failed: %zd != %" PRIu64 " %s (%d)\n", s, exp, strerror(errno), errno);
// 			break;
// 		}
// 		
// 		event = create_event(CRTX_EVT_TIMER, 0, 0);
// 		event->data.raw.uint32 = exp;
// 		event->data.raw.type = 'u';
// 		event->data.raw.flags = DIF_DATA_UNALLOCATED;
// 		
// 		add_event(tlist->parent.graph, event);
// 		
// 		if (tlist->newtimer.it_interval.tv_sec == 0 && tlist->newtimer.it_interval.tv_nsec == 0)
// 			break;
// 	}
// 	
// 	stop_thread(0, data);
// 	
// 	return 0;
// }

static char timer_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *payload;
	struct crtx_timer_listener *tlist;
	uint64_t exp;
	ssize_t s;
	
	
	payload = (struct crtx_event_loop_payload*) event->data.raw.pointer;
	
	tlist = (struct crtx_timer_listener *) payload->data;
	
	s = read(tlist->fd, &exp, sizeof(uint64_t));
	if (s == -1 && errno == EINTR) {
		DBG("timerfd thread interrupted\n");
		return 0;
	}
	if (s != sizeof(uint64_t)) {
		ERROR("reading from timerfd failed: %zd != %" PRIu64 " %s (%d)\n", s, exp, strerror(errno), errno);
		return 0;
	}
	
	event = create_event(CRTX_EVT_TIMER, 0, 0);
	event->data.raw.uint32 = exp;
	event->data.raw.type = 'u';
	event->data.raw.flags = DIF_DATA_UNALLOCATED;
	
	add_event(tlist->parent.graph, event);
	
// 	if (tlist->newtimer->it_interval.tv_sec == 0 && tlist->newtimer->it_interval.tv_nsec == 0)
// 		break;
	
	return 1;
}

void crtx_shutdown_timer_listener(struct crtx_listener_base *base) {
// 	stop_thread(0, data);
}

// static char start_listener(struct crtx_listener_base *base) {
// 	
// }

static char update_listener(struct crtx_listener_base *base) {
	struct crtx_timer_listener *tlist;
	int ret;
	
	tlist = (struct crtx_timer_listener *) base;
	printf("set %d %ld %ld\n", tlist->fd, tlist->newtimer.it_value.tv_sec, tlist->newtimer.it_interval.tv_sec);
	ret = timerfd_settime(tlist->fd, tlist->settime_flags, &tlist->newtimer, NULL);
	if (ret == -1) {
		ERROR("timerfd_settime failed: %s\n", strerror(errno));
		return 0;
	}
	
	return 1;
}

struct crtx_listener_base *crtx_new_timer_listener(void *options) {
	struct crtx_timer_listener *tlist;
// 	int ret;
// 	struct crtx_thread *t;
	
	tlist = (struct crtx_timer_listener *) options;
	
// 	if (!tlist->newtimer)
// 		return 0;
	
// 	new_eventgraph(&tlist->parent.graph, 0, 0);
	
	tlist->fd = timerfd_create(tlist->clockid, 0);
	if (tlist->fd == -1) {
		ERROR("timerfd_create failed: %s\n", strerror(errno));
		return 0;
	}
	
// 	ret = timerfd_settime(tlist->fd, tlist->settime_flags, tlist->newtimer, NULL);
// 	if (ret == -1) {
// 		ERROR("timerfd_settime failed: %s\n", strerror(errno));
// 		return 0;
// 	}
	
	tlist->parent.el_payload.fd = tlist->fd;
	tlist->parent.el_payload.event_flags = EPOLLIN;
	tlist->parent.el_payload.data = tlist;
	tlist->parent.el_payload.event_handler = &timer_fd_event_handler;
	tlist->parent.el_payload.event_handler_name = "timer fd handler";
	
	tlist->parent.start_listener = &update_listener;
	tlist->parent.update_listener = &update_listener;
// 	tlist->parent.thread = get_thread(timer_tmain, tlist, 0);
// 	tlist->parent.thread->do_stop = &stop_thread;
// 	start_thread(t);
	
	tlist->parent.shutdown = &crtx_shutdown_timer_listener;
	
	return &tlist->parent;
}

static char restart_listener_task(struct crtx_event *event, void *userdata, void **sessiondata) {
	
	
	return 1;
}

struct crtx_listener_base *crtx_timer_restart_listener(void *options) {
	struct crtx_timer_listener *tlist;
	struct crtx_listener_base *blist;
	
	
	tlist = (struct crtx_timer_listener *) options;
	
// 	memset(&tlist, 0, sizeof(struct crtx_timer_listener));
// 	
// 	// set time for (first) alarm
// 	tlist.newtimer.it_value.tv_sec = 1;
// 	tlist.newtimer.it_value.tv_nsec = 0;
// 	
// 	// set interval for repeating alarm, set to 0 to disable repetition
// 	tlist.newtimer.it_interval.tv_sec = 1;
// 	tlist.newtimer.it_interval.tv_nsec = 0;
// 	
// 	tlist.clockid = CLOCK_REALTIME; // clock source, see: man clock_gettime()
// 	tlist.settime_flags = 0; // absolute (TFD_TIMER_ABSTIME), or relative (0) time, see: man timerfd_settime()
// 	// 	tlist.newtimer = &newtimer;
	
	blist = create_listener("timer", tlist);
	if (!blist) {
		ERROR("create_listener(timer) failed\n");
		exit(1);
	}
	
	crtx_create_task(blist->graph, 0, "restart_listener", restart_listener_task, 0);
	
// 	crtx_start_listener(blist);
	
	return blist;
}

void crtx_timer_init() {
}

void crtx_timer_finish() {
}

#ifdef CRTX_TEST
static char timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("received timer event: %u\n", event->data.raw.uint32);
	
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
	
	return 0;
}

CRTX_TEST_MAIN(timer_main);
#endif
