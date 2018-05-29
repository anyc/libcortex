#ifndef _CRTX_EPOLL_H
#define _CRTX_EPOLL_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <sys/epoll.h>

struct crtx_epoll_listener {
	struct crtx_listener_base parent;
	
	struct crtx_event_loop *evloop;
	
	int epoll_fd;
// 	int pipe_fds[2];
	
	size_t max_n_events;
	int timeout;
	
	char stop;
};

// void *crtx_epoll_main(void *data);
// void crtx_epoll_stop(struct crtx_epoll_listener *epl);
struct crtx_listener_base *crtx_new_epoll_listener(void *options);
void crtx_epoll_init();
void crtx_epoll_finish();


// char *epoll_flags2str(int *flags);
// struct crtx_evloop_fd * crtx_epoll_add_fd(struct crtx_listener_base *lbase, int fd, void *data, char *event_handler_name, crtx_handle_task_t event_handler);
// void crtx_epoll_add_fd(struct crtx_listener_base *lbase, struct crtx_evloop_fd *evloop_fd);
// void crtx_epoll_mod_fd(struct crtx_listener_base *lbase, struct crtx_evloop_fd *evloop_fd);
// void crtx_epoll_del_fd(struct crtx_listener_base *lbase, struct crtx_evloop_fd *evloop_fd);
// void crtx_epoll_queue_graph(struct crtx_epoll_listener *epl, struct crtx_graph *graph);

#endif
