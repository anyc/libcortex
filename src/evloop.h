#ifndef CRTX_EVLOOP_H
#define CRTX_EVLOOP_H

#ifdef __cplusplus
extern "C" {
#endif

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

// 
#define CRTX_timespec2uint64(ts) (((uint64_t)(ts)->tv_sec) * UINT64_C(1000000000) + (ts)->tv_nsec)
#define CRTX_timespec2us_u64(ts) (((uint64_t)(ts)->tv_sec) * UINT64_C(1000000) + (ts)->tv_nsec/1000)
#define CRTX_CMP_TIMESPEC(ts1, ts2) \
	( \
		( (ts1)->tv_sec - (ts2)->tv_sec ) ? ( (ts1)->tv_sec - (ts2)->tv_sec ) : \
		( (ts1)->tv_nsec - (ts2)->tv_nsec ) ? ( (ts1)->tv_nsec - (ts2)->tv_nsec ) : 0 \
		)

// deprecated, use with CRTX_ prefix
#define EVLOOP_READ (1<<0)
#define EVLOOP_WRITE (1<<1)
#define EVLOOP_SPECIAL (1<<2)
#define EVLOOP_EDGE_TRIGGERED (1<<3)
#define EVLOOP_TIMEOUT (1<<4)
#define EVLOOP_ERROR (1<<5)
#define EVLOOP_HUP (1<<6)
#define EVLOOP_RDHUP (1<<7)

#define CRTX_EVLOOP_READ (1<<0)
#define CRTX_EVLOOP_WRITE (1<<1)
#define CRTX_EVLOOP_SPECIAL (1<<2)
#define CRTX_EVLOOP_EDGE_TRIGGERED (1<<3)
#define CRTX_EVLOOP_TIMEOUT (1<<4)
#define CRTX_EVLOOP_ERROR (1<<5)
#define CRTX_EVLOOP_HUP (1<<6)
#define CRTX_EVLOOP_RDHUP (1<<7)
#define CRTX_EVLOOP_ALL (((1<<8)-1) & (~CRTX_EVLOOP_EDGE_TRIGGERED))

struct crtx_evloop_fd;
struct crtx_evloop_callback;
struct crtx_event_loop;

typedef void (*crtx_evloop_error_cb_t)(struct crtx_evloop_callback *el_cb, void *data);

#define CRTX_ECBF_FREE	(1<<0)

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
	
	int flags;
};

struct crtx_listener_base;

// This structure contains the necessary information for a file descriptor
// that will be added to the event loop. There can be one or more crtx_evloop_callback
// structures for this structure where each callback can be responsible for a different
// type of event for this file descripter (e.g., read or write).
struct crtx_evloop_fd {
	struct crtx_ll ll;
	
	int *fd;
	int l_fd;
	char fd_added;
	
	void *el_data; // data that can be set by the event loop implementation
	
	struct crtx_listener_base *listener;
	
	struct crtx_evloop_callback *callbacks;
	
	struct crtx_event_loop *evloop;
	
	#ifndef CRTX_REDUCED_SIZE
	/* reserved to avoid ABI breakage */
	void *reserved1;
	#endif
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
	
	char after_fork_close;
	
	// if set, keep processing events but shutdown if no more events available
	char phase_out;
	
	struct crtx_evloop_callback *startup_el_cb;
};

extern struct crtx_event_loop *crtx_event_loops;

// int crtx_evloop_add_el_fd(struct crtx_evloop_fd *el_fd);
// int crtx_evloop_set_el_fd(struct crtx_evloop_fd *el_fd);
struct crtx_event_loop* crtx_get_main_event_loop();
int crtx_get_event_loop(struct crtx_event_loop **evloop, const char *id);

void crtx_evloop_callback(struct crtx_evloop_callback *el_cb);
int crtx_evloop_queue_graph(struct crtx_event_loop *evloop, struct crtx_graph *graph);
int crtx_evloop_start(struct crtx_event_loop *evloop);
int crtx_evloop_stop(struct crtx_event_loop *evloop);
int crtx_evloop_release(struct crtx_event_loop *evloop);
int crtx_evloop_create_fd_entry(struct crtx_evloop_fd *evloop_fd, struct crtx_evloop_callback *el_callback,
						  int *fd_ptr, int fd,
						  int event_flags,
						  struct crtx_graph *graph,
						  crtx_handle_task_t event_handler,
						  void *data,
						  crtx_evloop_error_cb_t error_cb,
						  void *error_cb_data
					);
int crtx_evloop_init_listener(struct crtx_listener_base *listener,
						int *fd_ptr, int fd,
						int event_flags,
						struct crtx_graph *graph,
						crtx_handle_task_t event_handler,
						void *event_handler_data,
						crtx_evloop_error_cb_t error_cb,
						void *error_cb_data
					);
void crtx_evloop_update_default_fd(struct crtx_listener_base *listener, int fd);
int crtx_evloop_enable_cb(struct crtx_evloop_callback *el_cb);
int crtx_evloop_disable_cb(struct crtx_evloop_callback *el_cb);
int crtx_evloop_remove_cb(struct crtx_evloop_callback *el_cb);

void crtx_evloop_set_timeout(struct crtx_evloop_callback *el_cb, struct timespec *timeout);
void crtx_evloop_set_timeout_abs(struct crtx_evloop_callback *el_cb, clockid_t clockid, uint64_t timeout_us);
void crtx_evloop_set_timeout_rel(struct crtx_evloop_callback *el_cb, uint64_t timeout_us);
void crtx_evloop_disable_timeout(struct crtx_evloop_callback *el_cb);
int crtx_evloop_trigger_callback(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb);
int crtx_evloop_register_startup_callback(crtx_handle_task_t event_handler, void *userdata, struct crtx_event_loop *evloop);
int crtx_evloop_schedule_callback(crtx_handle_task_t event_handler, void *userdata, struct crtx_event_loop *evloop);

void crtx_event_flags2str(FILE *fd, unsigned int flags);

void crtx_evloop_handle_fd_closed(struct crtx_evloop_fd *evloop_fd);

#ifdef __cplusplus
}
#endif

#endif
