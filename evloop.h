#ifndef CRTX_EVLOOP_H
#define CRTX_EVLOOP_H

/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

#include "intern.h"

#define EVLOOP_READ 1<<0
#define EVLOOP_WRITE 1<<1

struct crtx_event_loop_payload {
	struct crtx_ll ll;
	
	int fd;
	int crtx_event_flags;
	char fd_added;
	char active;
	
	void *data;
	
	crtx_handle_task_t event_handler;
	char *event_handler_name;
	int triggered_flags;
	
	void (*simple_callback)(struct crtx_event_loop_payload *el_payload);
	void (*error_cb)(struct crtx_event_loop_payload *el_payload, void *data);
	void *error_cb_data;
	
	void *el_data;
	
	struct crtx_event_loop_payload *parent;
	struct crtx_ll *sub_payloads;
// 	struct crtx_event_loop_payload *epollin;
// 	struct crtx_event_loop_payload *epollout;
// 	struct crtx_event_loop_payload *epollpri;
};

struct crtx_listener_base;

struct crtx_event_loop {
	struct crtx_epoll_listener *listener;
	
	void (*add_fd)(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload);
	void (*mod_fd)(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload);
	void (*del_fd)(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload);
	
	char start_thread;
};

struct crtx_event_loop* crtx_get_event_loop();

#endif