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

struct crtx_evloop_fd;
struct crtx_evloop_callback;


typedef void (*crtx_evloop_error_cb_t)(struct crtx_evloop_callback *el_cb, void *data);

struct crtx_evloop_callback {
	struct crtx_ll ll;
	
	int crtx_event_flags; // event flags set using libcortex #define's
	char active;
	
	void *data; // data that can be set by the specific listener
	void *el_data; // data that can be set by the event loop implementation
	
	int triggered_flags;
	
	struct crtx_graph *graph;
	crtx_handle_task_t event_handler;
	char *event_handler_name;
	
	crtx_evloop_error_cb_t error_cb;
	void *error_cb_data;
	
	struct crtx_evloop_fd *fd_entry;
};

struct crtx_evloop_fd {
	struct crtx_ll ll;
	
	int fd;
	char fd_added;
	
	void *el_data; // data that can be set by the event loop implementation
	
	struct crtx_evloop_callback *callbacks;
};

struct crtx_listener_base;

struct crtx_event_loop_control_pipe {
	struct crtx_graph *graph;
};

struct crtx_event_loop {
	struct crtx_ll ll;
	
	const char *id;
	
	int ctrl_pipe[2];
	struct crtx_evloop_fd ctrl_pipe_evloop_handler;
	struct crtx_evloop_callback default_el_cb;
	
	struct crtx_listener_base *listener;
	
	int (*create)(struct crtx_event_loop *evloop);
	int (*release)(struct crtx_event_loop *evloop);
	int (*start)(struct crtx_event_loop *evloop);
	int (*stop)(struct crtx_event_loop *evloop);
	
	int (*mod_fd)(struct crtx_event_loop *evloop, struct crtx_evloop_fd *evloop_fd);
};

extern struct crtx_event_loop *crtx_event_loops;

struct crtx_event_loop* crtx_get_main_event_loop();
int crtx_get_event_loop(struct crtx_event_loop *evloop, const char *id);

void crtx_evloop_callback(struct crtx_evloop_callback *el_cb);
int crtx_evloop_queue_graph(struct crtx_event_loop *evloop, struct crtx_graph *graph);
int crtx_evloop_start(struct crtx_event_loop *evloop);
int crtx_evloop_stop(struct crtx_event_loop *evloop);
int crtx_evloop_release(struct crtx_event_loop *evloop);
int crtx_evloop_create_fd_entry(struct crtx_evloop_fd *evloop_fd, struct crtx_evloop_callback *el_callback,
						  int fd,
						  int event_flags,
						  struct crtx_graph *graph,
						  crtx_handle_task_t event_handler,
						  void *data,
						  crtx_evloop_error_cb_t error_cb
					);
int crtx_evloop_init_listener(struct crtx_listener_base *listener,
						int fd,
						int event_flags,
						struct crtx_graph *graph,
						crtx_handle_task_t event_handler,
						void *data,
						crtx_evloop_error_cb_t error_cb
					);
int crtx_evloop_enable_cb(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb);
int crtx_evloop_disable_cb(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb);
int crtx_evloop_remove_cb(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb);
#endif
