#ifndef _CRTX_EPOLL_H
#define _CRTX_EPOLL_H

#ifdef __cplusplus
extern "C" {
#endif

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
	
	struct epoll_event *events;
	int n_rdy_events;
	int cur_event_idx;
	
	char stop;
};

struct crtx_listener_base *crtx_setup_epoll_listener(void *options);
void crtx_epoll_init();
void crtx_epoll_finish();

#ifdef __cplusplus
}
#endif

#endif
