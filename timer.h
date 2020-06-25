#ifndef _CRTX_TIMER_H
#define _CRTX_TIMER_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include "core.h"

#define CRTX_EVT_TIMER "cortex.timer"

// for settime_flags
// #include <sys/timerfd.h>

struct crtx_timer_listener {
	struct crtx_listener_base base;
	
	int fd;
// 	char stop;
// 	char oneshot;
	crtx_handle_task_t oneshot_callback;
	void *oneshot_data;
	
	
	int clockid;
	struct itimerspec newtimer;
	int settime_flags;
};

struct crtx_timer_retry_listener {
	struct crtx_timer_listener timer_lstnr;
	
	struct crtx_listener_base *lstnr;
	
	unsigned int seconds;
};

struct crtx_timer_retry_listener *crtx_timer_retry_listener(struct crtx_listener_base *lstnr, unsigned int seconds);
void crtx_timer_retry_listener_free(struct crtx_timer_retry_listener *retry_lstnr);
int crtx_timer_get_listener(struct crtx_timer_listener *tlist,
							time_t offset_sec,
							long offset_nsec,
							time_t int_sec,
							long int_nsec
						);

int crtx_timer_oneshot(time_t offset_sec,
					   long offset_nsec,
					   int clockid,
					   crtx_handle_task_t callback,
					   void *callback_userdata
					);

struct crtx_listener_base *crtx_setup_timer_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(timer)

void crtx_timer_init();
void crtx_timer_finish();

#endif
