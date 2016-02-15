
struct crtx_timer_listener {
	struct crtx_listener_base parent;
	
	int fd;
	char stop;
	
	int clockid;
	const struct itimerspec *newtimer;
	int settime_flags;
};

void crtx_timer_init();
void crtx_timer_finish();
struct crtx_listener_base *crtx_new_timer_listener(void *options);