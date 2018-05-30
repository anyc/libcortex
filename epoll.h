#ifndef _CRTX_EPOLL_H
#define _CRTX_EPOLL_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <sys/epoll.h>

struct crtx_epoll_listener {
	struct crtx_listener_base base;
	
	struct crtx_event_loop *evloop;
	
	int epoll_fd;
	size_t max_n_events;
	int timeout;
	
	char stop;
};

struct crtx_listener_base *crtx_new_epoll_listener(void *options);
void crtx_epoll_init();
void crtx_epoll_finish();

#endif
