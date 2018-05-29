/*
 * Mario Kicherer (dev@kicherer.org)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "core.h"
#include "evloop.h"

// #include <sys/epoll.h>
// #ifdef CRTX_EVLOOP_X
// #else
// #include "epoll.h"
// #endif

struct crtx_event_loop *crtx_event_loops = 0;

static char evloop_ctrl_pipe_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_evloop_callback *el_cb;
	struct crtx_event_loop_control_pipe ecp;
	
	
	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	
	VDBG("received wake-up event\n");
	
	read(el_cb->fd_entry->fd, &ecp, sizeof(struct crtx_event_loop_control_pipe));
	
	if (ecp.graph) {
		crtx_process_graph_tmain(ecp.graph);
		ecp.graph = 0;
	}
	
	return 0;
}

int crtx_evloop_create_fd_entry(struct crtx_evloop_fd *evloop_fd, struct crtx_evloop_callback *el_cb,
						  int fd,
						  int event_flags,
						  struct crtx_graph *graph,
						  crtx_handle_task_t event_handler,
						  void *data,
						  crtx_evloop_error_cb_t error_cb
						 )
{
// 	struct crtx_evloop_callback *el_cb;
	
	evloop_fd->fd = fd;
	
// 	el_cb = (struct crtx_evloop_callback *) calloc(1, sizeof(struct crtx_evloop_callback));
	
	el_cb->crtx_event_flags = event_flags;
	el_cb->graph = graph;
	el_cb->event_handler = event_handler;
	el_cb->data = data;
	el_cb->fd_entry = evloop_fd;
	el_cb->error_cb = error_cb;
	
	crtx_ll_append((struct crtx_ll**) &evloop_fd->callbacks, &el_cb->ll);
	
// 	if (el_callback)
// 		*el_callback = el_cb;
	
	return 0;
}

int crtx_evloop_init_listener(struct crtx_listener_base *listener,
						  int fd,
						  int event_flags,
						  struct crtx_graph *graph,
						  crtx_handle_task_t event_handler,
						  void *data,
						  crtx_evloop_error_cb_t error_cb
						)
{
	int ret;
	
	ret = crtx_evloop_create_fd_entry(&listener->evloop_fd, &listener->default_el_cb,
						fd,
						event_flags,
						graph,
						event_handler,
						data,
						error_cb
					);
	
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
// 	struct crtx_evloop_callback *cb_ll;
	
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
// 		evloop->ctrl_pipe_evloop_handler.crtx_event_flags = EVLOOP_READ;
// 		evloop->ctrl_pipe_evloop_handler.fd = evloop->ctrl_pipe[0];
// 		evloop->ctrl_pipe_evloop_handler.event_handler = &evloop_ctrl_pipe_handler;
		
		crtx_evloop_create_fd_entry(&evloop->ctrl_pipe_evloop_handler, &evloop->default_el_cb,
								evloop->ctrl_pipe[0],
								EVLOOP_READ,
								0,
								&evloop_ctrl_pipe_handler,
								0,
								0
							);
		
// 		evloop->add_fd(evloop, &evloop->ctrl_pipe_evloop_handler);
// 		evloop->mod_fd(evloop, &evloop->ctrl_pipe_evloop_handler);
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
// 				ret = crtx_root->event_loop.create(&crtx_root->event_loop);
// 				if (ret) {
// 					ERROR("creating eventloop %s failed\n", id);
// 					return 0;
// 				}
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
// 	printf("get main loop %p\n", crtx_root->event_loop.listener);
	if (!crtx_root->event_loop.listener) {
// 		struct crtx_listener_base *lbase;
		int ret;
		char *chosen_evloop;
// 		struct epoll_event ctrl_event;
		
		chosen_evloop = getenv("CRTX_EVLOOP");
		
		if (!chosen_evloop) {
			chosen_evloop = DEFAULT_EVLOOP;
		}
		
		crtx_get_event_loop(&crtx_root->event_loop, chosen_evloop);
		
		
// 		ret = pipe(crtx_root->event_loop->ctrl_pipe); // 0 read, 1 write
// 		if (ret < 0) {
// 			ERROR("creating control pipe failed: %s\n", strerror(errno));
// 		} else {
// // 			memset(&ctrl_event, 0, sizeof(struct epoll_event));
// // 			ctrl_event.events = EPOLLIN;
// // 			ctrl_event.data.ptr = 0;
// // 			
// // 			crtx_epoll_add_fd_intern(epl, epl->pipe_fds[0], &ctrl_event);
// 			
// 			crtx_root->event_loop->ctrl_pipe.evloop_handler.crtx_event_flags = EVLOOP_READ;
// 			crtx_root->event_loop.add_fd(crtx_root->event_loop->ctrl_pipe[0], &crtx_root->event_loop->ctrl_pipe_evloop_handler);
// 		}
		
// 		#ifdef CRTX_EVLOOP_X
// 		#else
// 		crtx_root->event_loop.listener = (struct crtx_epoll_listener*) calloc(1, sizeof(struct crtx_epoll_listener));
// 		#endif
// 		
// 		crtx_root->event_loop.add_fd = &crtx_epoll_add_fd;
// 		crtx_root->event_loop.mod_fd = &crtx_epoll_mod_fd;
// 		crtx_root->event_loop.del_fd = &crtx_epoll_del_fd;
// 		
// 		lbase = create_listener("epoll", crtx_root->event_loop.listener);
// 		if (!lbase) {
// 			ERROR("create_listener(epoll) failed\n");
// 			exit(1);
// 		}
		
		if (crtx_root->detached_event_loop) {
// 			ret = crtx_start_listener(&crtx_root->event_loop.parent);
			ret = crtx_evloop_start(&crtx_root->event_loop);
			if (ret) {
				ERROR("starting epoll listener failed\n");
				return 0;
			}
		}
	} 
	
	return &crtx_root->event_loop;
}

// void crtx_evloop_add_fd(struct crtx_event_loop *evloop, struct crtx_evloop_fd *evloop_fd) {
// 	evloop->add_fd(
// }
// 
// void crtx_evloop_mod_fd(struct crtx_event_loop *evloop, struct crtx_evloop_fd *evloop_fd) {
// 	
// }
// 
// void crtx_evloop_del_fd(struct crtx_event_loop *evloop, struct crtx_evloop_fd *evloop_fd) {
// 	
// }

int crtx_evloop_queue_graph(struct crtx_event_loop *evloop, struct crtx_graph *graph) {
	struct crtx_event_loop_control_pipe ecp;
	
	ecp.graph = graph;
// 	write(epl->pipe_fds[1], &ecp, sizeof(struct crtx_event_loop_control_pipe));
	write(evloop->ctrl_pipe[1], &ecp, sizeof(struct crtx_event_loop_control_pipe));
	
	return 0;
}

int crtx_evloop_start(struct crtx_event_loop *evloop) {
// 	crtx_epoll_main(evloop->listener);
	return evloop->start(evloop);
}

int crtx_evloop_stop(struct crtx_event_loop *evloop) {
// 	crtx_epoll_stop(evloop->listener);
	evloop->stop(evloop);
	
	return 0;
}

int crtx_evloop_release(struct crtx_event_loop *evloop) {
	evloop->stop(evloop);
	evloop->release(evloop);
	printf("release ctrl pipe\n");
	close(evloop->ctrl_pipe[1]);
	close(evloop->ctrl_pipe[0]);
	
	return 0;
}
