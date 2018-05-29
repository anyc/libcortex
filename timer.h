#ifndef _CRTX_TIMER_H
#define _CRTX_TIMER_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#define CRTX_EVT_TIMER "cortex.timer"

struct crtx_timer_listener {
	struct crtx_listener_base base;
	
	int fd;
	char stop;
	
	int clockid;
	struct itimerspec newtimer;
	int settime_flags;
};

struct crtx_timer_retry_listener {
	struct crtx_timer_listener timer_lstnr;
	
	struct crtx_listener_base *lstnr;
	
	unsigned int seconds;
};

void crtx_timer_init();
void crtx_timer_finish();
struct crtx_listener_base *crtx_new_timer_listener(void *options);

struct crtx_timer_retry_listener *crtx_timer_retry_listener(struct crtx_listener_base *lstnr, unsigned int seconds);
void crtx_timer_retry_listener_free(struct crtx_timer_retry_listener *retry_lstnr);
struct crtx_listener_base *crtx_timer_get_listener(struct crtx_timer_listener *tlist, time_t offset_sec, long offset_nsec, time_t int_sec, long int_nsec);
#endif
