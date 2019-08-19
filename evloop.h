#ifndef CRTX_EVLOOP_H
#define CRTX_EVLOOP_H

/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

#include <stdio.h>
#include <sys/time.h>

#include "common.h"

#ifndef CRTX_DEFAULT_CLOCK
#define CRTX_DEFAULT_CLOCK CLOCK_BOOTTIME
#endif

#define CRTX_timespec2uint64(ts) ((uint64_t)(ts)->tv_sec * 1000000000ULL + (ts)->tv_nsec)

#define EVLOOP_READ (1<<0)
#define EVLOOP_WRITE (1<<1)
#define EVLOOP_SPECIAL (1<<2)
#define EVLOOP_EDGE_TRIGGERED (1<<3)
#define EVLOOP_TIMEOUT (1<<4)

struct crtx_evloop_fd;
struct crtx_evloop_callback;
struct crtx_event_loop;

typedef void (*crtx_evloop_error_cb_t)(struct crtx_evloop_callback *el_cb, void *data);

struct crtx_evloop_callback {
	struct crtx_ll ll;
	
	int crtx_event_flags; // event flags set using libcortex #define's
	char active;
	char timeout_enabled;
	struct timespec timeout;
	
	void *el_data; // data that can be set by the event loop implementation
	
	int triggered_flags;
	
	struct crtx_graph *graph;
	crtx_handle_task_t event_handler;
	char *event_handler_name;
	void *event_handler_data; // data that can be set by the specific listener
	
	crtx_evloop_error_cb_t error_cb;
	void *error_cb_data;
	
	struct crtx_evloop_fd *fd_entry;
};

struct crtx_listener_base;

struct crtx_evloop_fd {
	struct crtx_ll ll;
	
	int fd;
	char fd_added;
	
	void *el_data; // data that can be set by the event loop implementation
	
	struct crtx_listener_base *listener;
	
	struct crtx_evloop_callback *callbacks;
	
	struct crtx_event_loop *evloop;
};

struct crtx_event_loop_control_pipe {
	struct crtx_graph *graph;
	struct crtx_evloop_callback *el_cb;
};

struct crtx_event_loop {
	struct crtx_ll ll;
	
	const char *id;
	
	int ctrl_pipe[2];
	struct crtx_evloop_fd ctrl_pipe_evloop_handler;
	struct crtx_evloop_callback default_el_cb;
	
	struct crtx_listener_base *listener;
	
	struct crtx_evloop_fd *fds;
	
	int (*create)(struct crtx_event_loop *evloop);
	int (*release)(struct crtx_event_loop *evloop);
	int (*start)(struct crtx_event_loop *evloop);
	int (*stop)(struct crtx_event_loop *evloop);
	int (*onetime)(struct crtx_event_loop *evloop);
	
	int (*mod_fd)(struct crtx_event_loop *evloop, struct crtx_evloop_fd *evloop_fd);
};

extern struct crtx_event_loop *crtx_event_loops;

int crtx_evloop_add_el_fd(struct crtx_evloop_fd *el_fd);
int crtx_evloop_set_el_fd(struct crtx_evloop_fd *el_fd);
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
						  crtx_evloop_error_cb_t error_cb,
						  void *error_cb_data
					);
int crtx_evloop_init_listener(struct crtx_listener_base *listener,
						int fd,
						int event_flags,
						struct crtx_graph *graph,
						crtx_handle_task_t event_handler,
						void *event_handler_data,
						crtx_evloop_error_cb_t error_cb,
						void *error_cb_data
					);
int crtx_evloop_enable_cb(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb);
int crtx_evloop_disable_cb(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb);
int crtx_evloop_remove_cb(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb);

void crtx_evloop_set_timeout(struct crtx_evloop_callback *el_cb, struct timespec *timeout);
void crtx_evloop_set_timeout_abs(struct crtx_evloop_callback *el_cb, clockid_t clockid, uint64_t timeout_us);
void crtx_evloop_set_timeout_rel(struct crtx_evloop_callback *el_cb, uint64_t timeout_us);
void crtx_evloop_disable_timeout(struct crtx_evloop_callback *el_cb);
int crtx_evloop_trigger_callback(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb);

void crtx_event_flags2str(FILE *fd, unsigned int flags);

#endif
