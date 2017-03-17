
#ifndef _CRTX_TIMER_H
#define _CRTX_TIMER_H

#define CRTX_EVT_TIMER "cortex.timer"

struct crtx_timer_listener {
	struct crtx_listener_base parent;
	
	int fd;
	char stop;
	
	int clockid;
	struct itimerspec newtimer;
	int settime_flags;
};

void crtx_timer_init();
void crtx_timer_finish();
struct crtx_listener_base *crtx_new_timer_listener(void *options);

#endif
