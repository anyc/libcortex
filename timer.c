/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

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

static char timer_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_timer_listener *tlist;
	uint64_t exp;
	ssize_t s;
	
// 	struct crtx_evloop_callback *el_cb;
	
// 	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	
	tlist = (struct crtx_timer_listener *) userdata;
	
	if (tlist->oneshot_callback) {
		tlist->oneshot_callback(0, tlist->oneshot_data, 0);
		
		crtx_free_listener(&tlist->base);
		
		return 1;
	}
	
	s = read(tlist->fd, &exp, sizeof(uint64_t));
	if (s == -1 && errno == EINTR) {
		CRTX_DBG("timerfd thread interrupted\n");
		return 0;
	}
	if (s != sizeof(uint64_t)) {
		CRTX_ERROR("reading from timerfd failed: %zd != %" PRIu64 " %s (%d)\n", s, exp, strerror(errno), errno);
		return 0;
	}
	
	s = crtx_create_event(&event);
	if (s) {
		CRTX_ERROR("crtx_create_event failed: %s\n", strerror(s));
		return 1;
	}
	
	event->description = CRTX_EVT_TIMER;
	crtx_event_set_raw_data(event, 'U', exp, sizeof(exp), 0);
	
	crtx_add_event(tlist->base.graph, event);
	
	return 1;
}

static char update_listener(struct crtx_listener_base *base) {
	struct crtx_timer_listener *tlist;
	int ret;
	
	tlist = (struct crtx_timer_listener *) base;
	
	CRTX_DBG("timer: update fd %d (%ld.%09lds %ld.%09lds clockId %d)\n", tlist->fd,
			tlist->newtimer.it_value.tv_sec, tlist->newtimer.it_value.tv_nsec, 
			tlist->newtimer.it_interval.tv_sec, tlist->newtimer.it_interval.tv_nsec,
			tlist->clockid
		);
	
	ret = timerfd_settime(tlist->fd, tlist->settime_flags, &tlist->newtimer, NULL);
	if (ret == -1) {
		CRTX_ERROR("timerfd_settime failed: %s\n", strerror(errno));
		return ret;
	}
	
	return 0;
}

static void shutdown_listener(struct crtx_listener_base *base) {
	base->start_listener = 0;
	
	close(base->evloop_fd.fd);
	base->evloop_fd.fd = 0;
}

struct crtx_listener_base *crtx_setup_timer_listener(void *options) {
	struct crtx_timer_listener *tlist;
	
	tlist = (struct crtx_timer_listener *) options;
	
	if (tlist->base.start_listener != &update_listener) {
		tlist->fd = timerfd_create(tlist->clockid, 0);
		if (tlist->fd == -1) {
			CRTX_ERROR("timerfd_create failed: %s\n", strerror(errno));
			return 0;
		}
		
		crtx_evloop_init_listener(&tlist->base,
							tlist->fd,
						EVLOOP_READ,
						0,
						&timer_fd_event_handler,
						tlist,
						0, 0
						);
		
		tlist->base.start_listener = &update_listener;
		tlist->base.update_listener = &update_listener;
		tlist->base.shutdown = &shutdown_listener;
	} else {
		update_listener(&tlist->base);
	}
	
	return &tlist->base;
}



/*
 * retry listener that tries to restart stopped listeners after a given time
 */

static char retry_listener_task(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_timer_retry_listener *retry_lstnr;
	
	
	retry_lstnr = (struct crtx_timer_retry_listener*) userdata;
	
	if (retry_lstnr->lstnr->state == CRTX_LSTNR_STOPPED) {
		crtx_start_listener(retry_lstnr->lstnr);
	}
	
	return 1;
}

struct crtx_timer_retry_listener *crtx_timer_retry_listener(struct crtx_listener_base *lstnr, unsigned int seconds) {
	struct crtx_timer_listener *tlist;
	struct crtx_timer_retry_listener *retry_lstnr;
	int r;
	
	
	retry_lstnr = (struct crtx_timer_retry_listener*) calloc(1, sizeof(struct crtx_timer_retry_listener));
	retry_lstnr->lstnr = lstnr;
	
	tlist = &retry_lstnr->timer_lstnr;
	tlist->newtimer.it_value.tv_sec = seconds;
	tlist->newtimer.it_value.tv_nsec = 0;
	tlist->newtimer.it_interval.tv_sec = seconds;
	tlist->newtimer.it_interval.tv_nsec = 0;
	tlist->clockid = CLOCK_REALTIME;
	
	r = crtx_setup_listener("timer", tlist);
	if (r) {
		CRTX_ERROR("create_listener(timer) failed: %s\n", strerror(-r));
		
		free(retry_lstnr);
		
		return 0;
	}
	
	crtx_create_task(tlist->base.graph, 0, "retry_lstnr_task", &retry_listener_task, retry_lstnr);
	
	return retry_lstnr;
}

void crtx_timer_retry_listener_free(struct crtx_timer_retry_listener *retry_lstnr) {
	crtx_stop_listener(&retry_lstnr->timer_lstnr.base);
	
	free(retry_lstnr);
}


/*
 * convenience functions
 */

int crtx_timer_get_listener(struct crtx_timer_listener *tlist,
							time_t offset_sec,
							long offset_nsec,
							time_t int_sec,
							long int_nsec
							)
{
	int r;
	
	
	memset(tlist, 0, sizeof(struct crtx_timer_listener));
	
	tlist->newtimer.it_value.tv_sec = offset_sec;
	tlist->newtimer.it_value.tv_nsec = offset_nsec;
	
	tlist->newtimer.it_interval.tv_sec = int_sec;
	tlist->newtimer.it_interval.tv_nsec = int_nsec;
	
	tlist->clockid = CLOCK_REALTIME;
	tlist->settime_flags = 0;
	
	r = crtx_setup_listener("timer", tlist);
	if (r) {
		CRTX_ERROR("create_listener(timer) failed: %s\n", strerror(-r));
		return 0;
	}
	
	return 0;
}

int crtx_timer_oneshot(time_t offset_sec,
						long offset_nsec,
						int clockid,
						crtx_handle_task_t callback,
						void *callback_userdata
						)
{
	struct crtx_timer_listener *tlist;
	int r;
	
	
	tlist = (struct crtx_timer_listener*) malloc(sizeof(struct crtx_timer_listener));
	
	memset(tlist, 0, sizeof(struct crtx_timer_listener));
	
	tlist->newtimer.it_value.tv_sec = offset_sec;
	tlist->newtimer.it_value.tv_nsec = offset_nsec;
	
	if (clockid < 0)
		tlist->clockid = CLOCK_REALTIME;
	else
		tlist->clockid = clockid;
	tlist->oneshot_callback = callback;
	tlist->oneshot_data = callback_userdata;
	
	r = crtx_setup_listener("timer", tlist);
	if (r) {
		CRTX_ERROR("create_listener(timer) failed: %s\n", strerror(-r));
		return 0;
	}
	
	crtx_start_listener(&tlist->base);
	
	return 0;
}

CRTX_DEFINE_ALLOC_FUNCTION(timer)

void crtx_timer_init() {
}

void crtx_timer_finish() {
}




#ifdef CRTX_TEST
static char timertest_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("received timer event: %u\n", event->data.uint32);
	
	return 1;
}

int timer_main(int argc, char **argv) {
	struct crtx_timer_listener tlist;
// 	struct itimerspec newtimer;
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
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(timer_main);
#endif
