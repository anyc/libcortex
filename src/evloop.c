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

void crtx_event_flags2str(FILE *f, unsigned int flags) {
	#define RETFLAG(flag) { if (flags & flag) fprintf(f, "%s ", #flag); }
	
	RETFLAG(CRTX_EVLOOP_READ);
	RETFLAG(CRTX_EVLOOP_WRITE);
	RETFLAG(CRTX_EVLOOP_SPECIAL);
	RETFLAG(CRTX_EVLOOP_EDGE_TRIGGERED);
	RETFLAG(CRTX_EVLOOP_TIMEOUT);
	RETFLAG(CRTX_EVLOOP_ERROR);
	RETFLAG(CRTX_EVLOOP_HUP);
	RETFLAG(CRTX_EVLOOP_RDHUP);
}

void crtx_evloop_set_timeout(struct crtx_evloop_callback *el_cb, struct timespec *timeout) {
	memcpy(&el_cb->timeout, timeout, sizeof(struct timespec));
}

void crtx_evloop_set_timeout_abs(struct crtx_evloop_callback *el_cb, clockid_t clockid, uint64_t timeout_us) {
	if (clockid != CRTX_DEFAULT_CLOCK) {
		struct timespec ts;
		uint64_t now_us;
		
		clock_gettime(clockid, &ts);
		now_us = CRTX_timespec2us_u64(&ts);
		
		if (now_us < timeout_us) {
			crtx_evloop_set_timeout_rel(el_cb, timeout_us - now_us);
		} else {
			crtx_evloop_set_timeout_rel(el_cb, 0);
		}
		
		return;
	}
	
	el_cb->timeout_enabled = 1;
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
	CRTX_VDBG("timeout in %" PRIu64 " (now %" PRIu64 ")\n", timeout_us >= now_us ? timeout_us - now_us : 0, now_us);
	#endif
}

void crtx_evloop_set_timeout_rel(struct crtx_evloop_callback *el_cb, uint64_t timeout_us) {
	#ifdef DEBUG
	CRTX_VDBG("timeout in %" PRIu64 " us\n", timeout_us);
	#endif
	
	clock_gettime(CRTX_DEFAULT_CLOCK, &el_cb->timeout);
	
	el_cb->timeout_enabled = 1;
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

void crtx_evloop_disable_timeout(struct crtx_evloop_callback *el_cb) {
	el_cb->timeout_enabled = 0;
}

static int evloop_ctrl_pipe_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_evloop_callback *el_cb;
	struct crtx_event_loop_control_pipe ecp;
	ssize_t r;
	int rv;
	
	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	
	CRTX_VDBG("received ctrl pipe event\n");
	
	r = read(*el_cb->fd_entry->fd, &ecp, sizeof(struct crtx_event_loop_control_pipe));
	
	if (r != sizeof(struct crtx_event_loop_control_pipe)) {
		CRTX_ERROR("unexpected number of bytes read in evloop_ctrl_pipe_handler(): %zd != %zd\n", r, sizeof(struct crtx_event_loop_control_pipe));
		return 0;
	}
	
	if (ecp.graph) {
		unsigned int i;
		
		for (i=0; i<crtx_root->n_graphs; i++) {
			if (crtx_root->graphs[i] == ecp.graph) {
				CRTX_DBG("processing graph %s %p\n", ecp.graph->name, ecp.graph);
				while (1) {
					rv = crtx_process_one_event(ecp.graph);
					if (rv < 0)
						break;
				}
				ecp.graph = 0;
				break;
			}
		}
		if (ecp.graph) {
			CRTX_DBG("ignoring ctrl pipe request for graph %p\n", ecp.graph);
		}
	}
	
	if (ecp.el_cb) {
// 		CRTX_DBG("executing el_cb %p %d\n", ecp.el_cb, ecp.el_cb->crtx_event_flags);
		
		ecp.el_cb->triggered_flags = ecp.el_cb->crtx_event_flags;
		crtx_evloop_callback(ecp.el_cb);
	}
	
	return 0;
}

void crtx_evloop_callback(struct crtx_evloop_callback *el_cb) {
	struct crtx_event *event;
	
	if (el_cb->crtx_event_flags != 0 && (el_cb->crtx_event_flags & el_cb->triggered_flags) == 0)
		return;
	
	if (el_cb->fd_entry)
		CRTX_VDBG("exec callback for fd %d\n", *el_cb->fd_entry->fd);
	else
		CRTX_VDBG("exec callback without fd\n");
	
	if (el_cb->graph || el_cb->event_handler) {
		crtx_create_event(&event);
		
		if (el_cb->fd_entry)
			event->origin = el_cb->fd_entry->listener;
		
		crtx_event_set_raw_data(event, 'p', el_cb, sizeof(el_cb), CRTX_DIF_DONT_FREE_DATA);
		
		if (el_cb->fd_entry && el_cb->fd_entry->listener && el_cb->fd_entry->listener->autolock_source)
			LOCK(el_cb->fd_entry->listener->source_lock);
		
		/*
		 * TODO in any case, we have to make sure that we read from the fd before
		 * we call epoll_wait() again as it will return again immediately (in default level-triggered
		 * mode) and we will create multiple events here for the same data in the fd	
		 */
		
// 		if (el_cb->crtx_event_flags & CRTX_EVLOOP_EDGE_TRIGGERED)
		
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
		
		if (el_cb->fd_entry && el_cb->fd_entry->listener && el_cb->fd_entry->listener->autolock_source)
			UNLOCK(el_cb->fd_entry->listener->source_lock);
		
		if (el_cb->flags & CRTX_ECBF_FREE)
			free(el_cb);
	} else {
		if (el_cb->fd_entry) {
			CRTX_ERROR("no handler for fd %d\n", *el_cb->fd_entry->fd);
		} else {
			CRTX_ERROR("crtx_evloop_callback(): no callback handler\n");
// 			exit(1);
		}
	}
}

void crtx_evloop_fwd_error2event_cb(struct crtx_evloop_callback *el_cb, void *data) {
	crtx_evloop_callback(el_cb);
}

void crtx_evloop_update_default_fd(struct crtx_listener_base *listener, int fd) {
	crtx_evloop_disable_cb(&listener->default_el_cb);
	
	*listener->evloop_fd.fd = fd;
	
	if (fd >= 0)
		crtx_evloop_enable_cb(&listener->default_el_cb);
}

void crtx_evloop_handle_fd_closed(struct crtx_evloop_fd *evloop_fd) {
	struct crtx_evloop_callback *el_cb;
	
	CRTX_DBG("received notification fd %d was closed\n", *evloop_fd->fd);
	
	for (el_cb=evloop_fd->callbacks; el_cb; el_cb = (struct crtx_evloop_callback *) el_cb->ll.next) {
		if (!el_cb->active)
			continue;
		
		el_cb->active = 0;
	}
	
	if (!evloop_fd->fd) {
		evloop_fd->fd = &evloop_fd->l_fd;
		evloop_fd->l_fd = -1;
	}
	
	if (evloop_fd->evloop)
		evloop_fd->evloop->mod_fd(evloop_fd->evloop, evloop_fd);
}

int crtx_evloop_create_fd_entry(struct crtx_evloop_fd *evloop_fd, struct crtx_evloop_callback *el_cb,
						  int *fd_ptr,
						  int fd,
						  int event_flags,
						  struct crtx_graph *graph,
						  crtx_handle_task_t event_handler,
						  void *event_handler_data,
						  crtx_evloop_error_cb_t error_cb,
						  void *error_cb_data
						 )
{
	if (fd_ptr) {
		evloop_fd->fd = fd_ptr;
	} else {
		evloop_fd->fd = &evloop_fd->l_fd;
		evloop_fd->l_fd = fd;
	}
	
	el_cb->crtx_event_flags = event_flags;
	el_cb->graph = graph;
	el_cb->event_handler = event_handler;
	el_cb->event_handler_data = event_handler_data;
	el_cb->fd_entry = evloop_fd;
	if (error_cb) {
		el_cb->error_cb = error_cb;
		el_cb->error_cb_data = error_cb_data;
	} else {
		el_cb->error_cb = &crtx_evloop_fwd_error2event_cb;
		el_cb->error_cb_data = 0;
	}
	
	crtx_ll_append((struct crtx_ll**) &evloop_fd->callbacks, &el_cb->ll);
	
	return 0;
}

int crtx_evloop_init_listener(struct crtx_listener_base *listener,
						  int *fd_ptr,
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
						fd_ptr,
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

int crtx_evloop_enable_cb(struct crtx_evloop_callback *el_cb) {
	struct crtx_evloop_fd *el_fd;
	
	
	el_fd = el_cb->fd_entry;
	
	el_cb->active = 1;
	
	if (el_fd->evloop == 0) {
		el_fd->evloop = crtx_get_main_event_loop();
		if (!el_fd->evloop) {
			CRTX_ERROR("no event loop for fd %d\n", *el_fd->fd);
			return -1;
		}
	}
	
	if (el_fd->fd_added == 0) {
// 		LOCK(el_fd->evloop->listener->dependencies_lock);
// 		LOCK(el_fd->listener->dependencies_lock);
		
// 		crtx_ll_append_new(&el_fd->evloop->listener->rev_dependencies, el_fd->listener);
// 		crtx_ll_append_new(&el_fd->listener->dependencies, el_fd->evloop->listener);
		
		el_fd->evloop->mod_fd(el_fd->evloop, el_cb->fd_entry);
		
// 		UNLOCK(el_fd->listener->dependencies_lock);
// 		UNLOCK(el_fd->evloop->listener->dependencies_lock);
	} else {
		el_fd->evloop->mod_fd(el_fd->evloop, el_cb->fd_entry);
	}
	
	return 0;
}

int crtx_evloop_disable_cb(struct crtx_evloop_callback *el_cb) {
	if (!el_cb->active) {
		CRTX_VDBG("crtx_evloop_disable_cb(): not active\n");
		return 0;
	}
	
	el_cb->active = 0;
	if (el_cb->fd_entry->evloop)
		el_cb->fd_entry->evloop->mod_fd(el_cb->fd_entry->evloop, el_cb->fd_entry);
	
	return 0;
}

int crtx_evloop_remove_cb(struct crtx_evloop_callback *el_cb) {
	crtx_evloop_disable_cb(el_cb);
	
	crtx_ll_unlink((struct crtx_ll**) &el_cb->fd_entry->callbacks, &el_cb->ll);
	
// 	if (!el_cb->fd_entry->callbacks) {
// 		// if the file descriptor has no more callbacks, remove the dependency between the
// 		// the corresponding listener and the event loop
// 		
// 		LOCK(evloop->listener->dependencies_lock);
// 		LOCK(el_cb->fd_entry->listener->dependencies_lock);
// 		
// 		crtx_ll_unlink(&evloop->listener->rev_dependencies, &el_cb->fd_entry->listener->ll);
// 		crtx_ll_unlink(&el_cb->fd_entry->listener->dependencies, &evloop->listener->ll);
// 		
// 		UNLOCK(el_cb->fd_entry->listener->dependencies_lock);
// 		UNLOCK(evloop->listener->dependencies_lock);
// 	}
	
	return 0;
}

int crtx_evloop_create(struct crtx_event_loop *evloop) {
	int ret;
	
	ret = evloop->create(evloop);
	if (ret) {
		CRTX_ERROR("creating eventloop %s failed\n", evloop->id);
		return ret;
	}
	
	ret = pipe(evloop->ctrl_pipe); // 0 read, 1 write
	
	if (ret < 0) {
		CRTX_ERROR("creating control pipe failed: %s\n", strerror(errno));
		return ret;
	} else {
		crtx_evloop_create_fd_entry(&evloop->ctrl_pipe_evloop_handler, &evloop->default_el_cb,
								&evloop->ctrl_pipe[0], 0,
								CRTX_EVLOOP_READ,
								0,
								&evloop_ctrl_pipe_handler,
								0,
								0,
								0
							);
		evloop->ctrl_pipe_evloop_handler.listener = evloop->listener;
		
		// set this here so crtx_evloop_enable_cb will not call crtx_get_main_event_loop()
		evloop->default_el_cb.fd_entry->evloop = evloop;
		
		crtx_evloop_enable_cb(&evloop->default_el_cb);
	}
	
	crtx_ll_append(&crtx_root->event_loops, &evloop->ll);
	evloop->ll.data = evloop;
	
	return 0;
}

// int crtx_evloop_finish(struct crtx_event_loop *evloop) {
// 	evloop->stop(evloop);
// 	
// 	crtx_shutdown_listener(evloop->listener);
// 	
// 	close(evloop->ctrl_pipe[1]);
// 	
// 	return 0;
// }

int crtx_get_event_loop(struct crtx_event_loop **evloop, const char *id) {
	struct crtx_ll *ll;
	
	*evloop = (struct crtx_event_loop *) calloc(1, sizeof(struct crtx_event_loop));
	
	for (ll=&crtx_event_loops->ll; ll; ll = ll->next) {
		if (!strcmp(((struct crtx_event_loop*)ll->data)->id, id)) {
			memcpy(*evloop, ll->data, sizeof(struct crtx_event_loop));
			
			break;
		}
	}
	if (!ll) {
		CRTX_ERROR("no event loop implementation found\n");
		return -1;
	}
	
	crtx_evloop_create(*evloop);
	
	return 0;
}

struct crtx_event_loop* crtx_get_main_event_loop() {
	if (!crtx_root->event_loop) {
		int ret;
		const char *chosen_evloop;
		
		chosen_evloop = getenv("CRTX_EVLOOP");
		
		if (!chosen_evloop) {
			chosen_evloop = crtx_root->chosen_event_loop;
		}
		
		if (!chosen_evloop) {
			chosen_evloop = DEFAULT_EVLOOP;
		}
		
		ret = crtx_get_event_loop(&crtx_root->event_loop, chosen_evloop);
		if (ret != 0) {
			return 0;
		}
		
		CRTX_DBG("setup event loop %s (%p)\n", chosen_evloop, crtx_root->event_loop);
		
// 		if (crtx_root->detached_event_loop) {
// 			ret = crtx_evloop_start(crtx_root->event_loop);
// 			if (ret) {
// 				CRTX_ERROR("starting epoll listener failed\n");
// 				return 0;
// 			}
// 		}
	}
	
	return crtx_root->event_loop;
}

int crtx_evloop_queue_graph(struct crtx_event_loop *evloop, struct crtx_graph *graph) {
	struct crtx_event_loop_control_pipe ecp;
	int rv;
	
	
	memset(&ecp, 0, sizeof(struct crtx_event_loop_control_pipe));
	
	ecp.graph = graph;
	rv = write(evloop->ctrl_pipe[1], &ecp, sizeof(struct crtx_event_loop_control_pipe));
	if (rv != sizeof(struct crtx_event_loop_control_pipe)) {
		CRTX_ERROR("write to selfpipe (queue_graph) failed: %d %zu\n", rv, sizeof(struct crtx_event_loop_control_pipe));
		return 1;
	}
	
	return 0;
}

int crtx_evloop_trigger_callback(struct crtx_event_loop *evloop, struct crtx_evloop_callback *el_cb) {
	struct crtx_event_loop_control_pipe ecp;
	int rv;
	
	memset(&ecp, 0, sizeof(struct crtx_event_loop_control_pipe));
	
	ecp.el_cb = el_cb;
	rv = write(evloop->ctrl_pipe[1], &ecp, sizeof(struct crtx_event_loop_control_pipe));
	if (rv != sizeof(struct crtx_event_loop_control_pipe)) {
		CRTX_ERROR("write to selfpipe (queue_graph) failed: %d %zu\n", rv, sizeof(struct crtx_event_loop_control_pipe));
		return 1;
	}
	
	return 0;
}

static struct crtx_evloop_callback default_evloop_startup_el_cb = {0};

int crtx_evloop_register_startup_callback(crtx_handle_task_t event_handler, void *userdata, struct crtx_event_loop *evloop) {
	if (!evloop)
		evloop = crtx_get_main_event_loop();
	
	evloop->startup_el_cb = &default_evloop_startup_el_cb;
	evloop->startup_el_cb->event_handler = event_handler;
	evloop->startup_el_cb->event_handler_data = userdata;
	
	crtx_evloop_trigger_callback(evloop, evloop->startup_el_cb);
	
	return 0;
}

int crtx_evloop_schedule_callback(crtx_handle_task_t event_handler, void *userdata, struct crtx_event_loop *evloop) {
	struct crtx_evloop_callback *el_cb;
	
	if (!evloop)
		evloop = crtx_get_main_event_loop();
	
	el_cb = (struct crtx_evloop_callback*) calloc(1, sizeof(struct crtx_evloop_callback));
	el_cb->event_handler = event_handler;
	el_cb->event_handler_data = userdata;
	el_cb->flags |= CRTX_ECBF_FREE;
	
	crtx_evloop_trigger_callback(evloop, el_cb);
	
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
	crtx_evloop_remove_cb(&evloop->default_el_cb);
	
	evloop->stop(evloop);
	evloop->release(evloop);
	
// 	printf("release ctrl pipe\n");
	
	if (evloop->ctrl_pipe[1] >= 0)
		close(evloop->ctrl_pipe[1]);
	if (evloop->ctrl_pipe[0] >= 0)
		close(evloop->ctrl_pipe[0]);
	
	crtx_ll_unlink(&crtx_root->event_loops, &evloop->ll);
	
	free(evloop);
	
	return 0;
}
