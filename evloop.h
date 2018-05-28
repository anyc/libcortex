#ifndef CRTX_EVLOOP_H
#define CRTX_EVLOOP_H

/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

#include "intern.h"

#define EVLOOP_READ 1<<0
#define EVLOOP_WRITE 1<<1
#define EVLOOP_SPECIAL 1<<2

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
};

struct crtx_listener_base;

struct crtx_event_loop_control_pipe {
	
	struct crtx_graph *graph;
};

struct crtx_event_loop {
	struct crtx_ll ll;
	
	const char *id;
	
// 	struct crtx_event_loop_control_pipe ctrl_pipe;
	int ctrl_pipe[2];
	struct crtx_event_loop_payload ctrl_pipe_evloop_handler;
	
	struct crtx_listener_base *listener;
	
	int (*create)(struct crtx_event_loop *evloop);
	int (*release)(struct crtx_event_loop *evloop);
	int (*start)(struct crtx_event_loop *evloop);
	int (*stop)(struct crtx_event_loop *evloop);
	
	int (*add_fd)(struct crtx_event_loop *evloop, struct crtx_event_loop_payload *el_payload);
	int (*mod_fd)(struct crtx_event_loop *evloop, struct crtx_event_loop_payload *el_payload);
	int (*del_fd)(struct crtx_event_loop *evloop, struct crtx_event_loop_payload *el_payload);
// 	int (*add_fd)(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload);
// 	int (*mod_fd)(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload);
// 	int (*del_fd)(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload);
};

extern struct crtx_event_loop *crtx_event_loops;

struct crtx_event_loop* crtx_get_main_event_loop();
int crtx_get_event_loop(struct crtx_event_loop *evloop, const char *id);

// void crtx_evloop_add_fd(struct crtx_event_loop *evloop, struct crtx_event_loop_payload *el_payload);
// void crtx_evloop_mod_fd(struct crtx_event_loop *evloop, struct crtx_event_loop_payload *el_payload);
// void crtx_evloop_del_fd(struct crtx_event_loop *evloop, struct crtx_event_loop_payload *el_payload);
int crtx_evloop_queue_graph(struct crtx_event_loop *evloop, struct crtx_graph *graph);
int crtx_evloop_start(struct crtx_event_loop *evloop);
int crtx_evloop_stop(struct crtx_event_loop *evloop);
int crtx_evloop_release(struct crtx_event_loop *evloop);

#endif
