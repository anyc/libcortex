/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>

#include "core.h"
#include "intern.h"
#include "evloop.h"

#ifndef TIME_T_MAX
_Static_assert( (sizeof(time_t) == sizeof(int32_t) || (sizeof(time_t) == sizeof(int64_t))), "unexpected time_t size");
#define TIME_T_MAX (sizeof(time_t) == sizeof(int32_t) ? INT32_MAX : (sizeof(time_t) == sizeof(int64_t)) ? INT64_MAX : 0)
#endif

struct crtx_event_loop *crtx_event_loops = 0;

void crtx_evloop_set_timeout(struct crtx_evloop_callback *el_cb, struct timespec *timeout) {
	memcpy(&el_cb->timeout, timeout, sizeof(struct timespec));
}

void crtx_evloop_set_timeout_abs(struct crtx_evloop_callback *el_cb, clockid_t clockid, uint64_t timeout_us) {
	if (clockid != CRTX_DEFAULT_CLOCK) {
		struct timespec ts;
		uint64_t now_us;
		
		clock_gettime(clockid, &ts);
		now_us = CRTX_timespec2uint64(&ts) / 1000;
		if (now_us < timeout_us) {
			crtx_evloop_set_timeout_rel(el_cb, timeout_us - now_us);
		} else {
			crtx_evloop_set_timeout_rel(el_cb, 0);
		}
		
		return;
	}
	
	if ((timeout_us / 1000000) >= TIME_T_MAX)
		el_cb->timeout.tv_sec = TIME_T_MAX;
	else
		el_cb->timeout.tv_sec = timeout_us / 1000000;
	
	el_cb->timeout.tv_nsec = (timeout_us % 1000000) * 1000;
	
	#ifdef DEBUG
	struct timespec ts;
	uint64_t now_us;
	
	clock_gettime(clockid, &ts);
	now_us = (ts.tv_sec * 1000000000ULL + ts.tv_nsec) / 1000;
	VDBG("timeout in %" PRIu64 " (now %" PRIu64 ")\n", timeout_us >= now_us ? timeout_us - now_us : 0, now_us);
	#endif
}

void crtx_evloop_set_timeout_rel(struct crtx_evloop_callback *el_cb, uint64_t timeout_us) {
	#ifdef DEBUG
	VDBG("timeout in %" PRIu64 " us\n", timeout_us);
	#endif
	
	clock_gettime(CRTX_DEFAULT_CLOCK, &el_cb->timeout);
	
	if ((timeout_us / 1000000) >= TIME_T_MAX) {
		el_cb->timeout.tv_sec = TIME_T_MAX;
	} else {
		if (el_cb->timeout.tv_sec > (TIME_T_MAX) - (timeout_us / 1000000)) {
			el_cb->timeout.tv_sec = TIME_T_MAX;
		} else {
			el_cb->timeout.tv_sec += (timeout_us / 1000000);
		}
	}
	
	el_cb->timeout.tv_nsec += (timeout_us % 1000000) * 1000;
	if (el_cb->timeout.tv_nsec >= 1000000000) {
		el_cb->timeout.tv_nsec -= 1000000000;
		if (el_cb->timeout.tv_sec < TIME_T_MAX)
			el_cb->timeout.tv_sec += 1;
	}
}

static char evloop_ctrl_pipe_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_evloop_callback *el_cb;
	struct crtx_event_loop_control_pipe ecp;
	ssize_t r;
	
	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	
	VDBG("received wake-up event\n");
	
	r = read(el_cb->fd_entry->fd, &ecp, sizeof(struct crtx_event_loop_control_pipe));
	
	if (r != sizeof(struct crtx_event_loop_control_pipe)) {
		ERROR("unexpected number of bytes read in evloop_ctrl_pipe_handler(): %zd != %zd\n", r, sizeof(struct crtx_event_loop_control_pipe));
		return 0;
	}
	
	if (ecp.graph) {
		crtx_process_graph_tmain(ecp.graph);
		ecp.graph = 0;
	}
	
	if (ecp.el_cb) {
		crtx_evloop_callback(ecp.el_cb);
	}
	
	return 0;
}

void crtx_evloop_callback(struct crtx_evloop_callback *el_cb) {
	struct crtx_event *event;
	
	if ((el_cb->crtx_event_flags & el_cb->triggered_flags) == 0)
		return;
	
	VDBG("received wakeup for fd %d\n", el_cb->fd_entry->fd);
	
	if (el_cb->graph || el_cb->event_handler) {
		event = crtx_create_event(0);
		
		crtx_event_set_raw_data(event, 'p', el_cb, sizeof(el_cb), CRTX_DIF_DONT_FREE_DATA);
		
		if (el_cb->fd_entry->listener && el_cb->fd_entry->listener->autolock_source)
			LOCK(el_cb->fd_entry->listener->source_lock);
		
		/*
		 * TODO in any case, we have to make sure that we read from the fd before
		 * we call epoll_wait() again as it will return again immediately (in default level-triggered
		 * mode) and we will create multiple events here for the same data in the fd	
		 */
		
// 		if (el_cb->crtx_event_flags & EVLOOP_EDGE_TRIGGERED)
		
		if (el_cb->event_handler) {
			el_cb->event_handler(event, el_cb->event_handler_data, 0);
			
			free_event(event);
		}
// 		} else {
// 			enum crtx_processing_mode mode;
// 			
// 			mode = crtx_get_mode(el_cb->graph->mode);
// 			
// 			// TODO process events here directly if mode == CRTX_PREFER_ELOOP ?
// 			if (mode == CRTX_PREFER_THREAD) {
// 				crtx_add_event(el_cb->graph, event);
// 			} else {
// 				crtx_add_event(el_cb->graph, event);
// 				
// // 				crtx_process_graph_tmain(graph);
// 			}
// 		}
		
		if (el_cb->fd_entry->listener && el_cb->fd_entry->listener->autolock_source)
			UNLOCK(el_cb->fd_entry->listener->source_lock);
	} else {
		ERROR("no handler for fd %d\n", el_cb->fd_entry->fd);
		exit(1);
	}
}

int crtx_evloop_create_fd_entry(struct crtx_evloop_fd *evloop_fd, struct crtx_evloop_callback *el_cb,
						  int fd,
						  int event_flags,
						  struct crtx_graph *graph,
						  crtx_handle_task_t event_handler,
						  void *event_handler_data,
						  crtx_evloop_error_cb_t error_cb,
						  void *error_cb_data
						 )
{
	evloop_fd->fd = fd;
	
	el_cb->crtx_event_flags = event_flags;
	el_cb->graph = graph;
	el_cb->event_handler = event_handler;
	el_cb->event_handler_data = event_handler_data;
	el_cb->fd_entry = evloop_fd;
	el_cb->error_cb = error_cb;
	el_cb->error_cb_data = error_cb_data;
	
	crtx_ll_append((struct crtx_ll**) &evloop_fd->callbacks, &el_cb->ll);
	
	return 0;
}

int crtx_evloop_init_listener(struct crtx_listener_base *listener,
						  int fd,
						  int event_flags,
						  struct crtx_graph *graph,
						  crtx_handle_task_t event_handler,
						  void *event_handler_data,
						  crtx_evloop_error_cb_t error_cb,
						  void *error_cb_data
						)
{
	int ret;
	
	ret = crtx_evloop_create_fd_entry(&listener->evloop_fd, &listener->default_el_cb,
						fd,
						event_flags,
						graph,
						event_handler,
						event_handler_data,
						error_cb,
						error_cb_data
					);
	
	listener->evloop_fd.listener = listener;
	listener->default_el_cb.active = 1;
	
	return ret;
}

int crtx_evloop_enable_cb(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb) {
	el_cb->active = 1;
	evloop->mod_fd(evloop, el_cb->fd_entry);
	
	return 0;
}

int crtx_evloop_disable_cb(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb) {
	if (!el_cb->active)
		return 0;
	
	el_cb->active = 0;
	evloop->mod_fd(evloop, el_cb->fd_entry);
	
	return 0;
}

int crtx_evloop_remove_cb(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb) {
	crtx_evloop_disable_cb(evloop, el_cb);
	
	crtx_ll_unlink((struct crtx_ll**) &el_cb->fd_entry->callbacks, &el_cb->ll);
	
	return 0;
}

int crtx_evloop_create(struct crtx_event_loop *evloop) {
	int ret;
	
	ret = evloop->create(evloop);
	if (ret) {
		ERROR("creating eventloop %s failed\n", evloop->id);
		return ret;
	}
	
	ret = pipe(evloop->ctrl_pipe); // 0 read, 1 write
	
	if (ret < 0) {
		ERROR("creating control pipe failed: %s\n", strerror(errno));
		return ret;
	} else {
		crtx_evloop_create_fd_entry(&evloop->ctrl_pipe_evloop_handler, &evloop->default_el_cb,
								evloop->ctrl_pipe[0],
								EVLOOP_READ,
								0,
								&evloop_ctrl_pipe_handler,
								0,
								0,
								0
							);
		
		crtx_evloop_enable_cb(evloop, &evloop->default_el_cb);
	}
	
	return 0;
}

int crtx_get_event_loop(struct crtx_event_loop *evloop, const char *id) {
	struct crtx_ll *ll;
	
	for (ll=&crtx_event_loops->ll; ll; ll = ll->next) {
		if (!strcmp(((struct crtx_event_loop*)ll->data)->id, id)) {
			memcpy(evloop, ll->data, sizeof(struct crtx_event_loop));
			
			break;
		}
	}
	if (!ll) {
		ERROR("no event loop implementation found\n");
		return 0;
	}
	
	crtx_evloop_create(evloop);
	
	return 0;
}

struct crtx_event_loop* crtx_get_main_event_loop() {
	if (!crtx_root->event_loop.id) {
		int ret;
		const char *chosen_evloop;
		
		chosen_evloop = getenv("CRTX_EVLOOP");
		
		if (!chosen_evloop) {
			chosen_evloop = crtx_root->chosen_event_loop;
		}
		
		if (!chosen_evloop) {
			chosen_evloop = DEFAULT_EVLOOP;
		}
		
		DBG("setup event loop %s\n", chosen_evloop);
		
		crtx_get_event_loop(&crtx_root->event_loop, chosen_evloop);
		
		if (crtx_root->detached_event_loop) {
			ret = crtx_evloop_start(&crtx_root->event_loop);
			if (ret) {
				ERROR("starting epoll listener failed\n");
				return 0;
			}
		}
	} 
	
	return &crtx_root->event_loop;
}

int crtx_evloop_queue_graph(struct crtx_event_loop *evloop, struct crtx_graph *graph) {
	struct crtx_event_loop_control_pipe ecp;
	
	memset(&ecp, 0, sizeof(struct crtx_event_loop_control_pipe));
	
	ecp.graph = graph;
	write(evloop->ctrl_pipe[1], &ecp, sizeof(struct crtx_event_loop_control_pipe));
	
	return 0;
}

int crtx_evloop_trigger_callback(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb) {
	struct crtx_event_loop_control_pipe ecp;
	
	memset(&ecp, 0, sizeof(struct crtx_event_loop_control_pipe));
	
	ecp.el_cb = el_cb;
	write(evloop->ctrl_pipe[1], &ecp, sizeof(struct crtx_event_loop_control_pipe));
	
	return 0;
}

int crtx_evloop_start(struct crtx_event_loop *evloop) {
	return evloop->start(evloop);
}

int crtx_evloop_stop(struct crtx_event_loop *evloop) {
	evloop->stop(evloop);
	
	return 0;
}

int crtx_evloop_release(struct crtx_event_loop *evloop) {
	evloop->stop(evloop);
	evloop->release(evloop);
	
// 	printf("release ctrl pipe\n");
	
	close(evloop->ctrl_pipe[1]);
	close(evloop->ctrl_pipe[0]);
	
	return 0;
}
