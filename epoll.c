
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "core.h"
#include "epoll.h"

int crtx_epoll_add_fd_intern(struct crtx_epoll_listener *epl, int fd, struct epoll_event *event) {
	int ret;
	
// 	event->events |= EPOLLET;
	
	ret = epoll_ctl(epl->epoll_fd, EPOLL_CTL_ADD, fd, event);
	if (ret < 0) {
		ERROR("epoll_ctl add failed for fd %d: %s\n", fd, strerror(errno));
		
		return ret;
	}
	
	return 0;
}

int crtx_epoll_mod_fd_intern(struct crtx_epoll_listener *epl, int fd, struct epoll_event *event) {
	int ret;
	
	if (epl->stop)
		return 0;
	
	ret = epoll_ctl(epl->epoll_fd, EPOLL_CTL_MOD, fd, event);
	if (ret < 0) {
		ERROR("epoll_ctl mod failed for fd %d: %s\n", fd, strerror(errno));
		
		return ret;
	}
	
	return 0;
}

int crtx_epoll_del_fd_intern(struct crtx_epoll_listener *epl, int fd) {
	int ret;
	
	if (epl->stop)
		return 0;
	
	ret = epoll_ctl(epl->epoll_fd, EPOLL_CTL_DEL, fd, 0);
	if (ret < 0) {
		ERROR("epoll_ctl del failed for fd %d: %s\n", fd, strerror(errno));
		
		return ret;
	}
	
	return 0;
}

// #include <sys/ioctl.h>
void crtx_epoll_add_fd(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload) {
	struct epoll_event *event;
	struct crtx_epoll_listener *epl;
	
	event = (struct epoll_event*) calloc(1, sizeof(struct epoll_event));
// 	event = &el_payload->event;
	el_payload->el_data = event;
	
	printf("epoll add %d %d\n", el_payload->fd, el_payload->event_flags);
	
// 	if (el_payload->event_flags == 0)
// 		event->events = EPOLLIN;
// 	else
		event->events = el_payload->event_flags;
	
// 	event->events = el_payload->event_flags;
	event->data.ptr = el_payload;
	
	epl = (struct crtx_epoll_listener *) lbase;
	
// 	crtx_create_task(lbase->graph, 0, el_payload->event_handler_name, el_payload->event_handler, 0);
	
// 	printf("add %d\n", el_payload->fd);
	
// 	int count;
// 	ioctl(el_payload->fd, FIONREAD, &count);
// 	printf("count %d\n", count);
	
	crtx_epoll_add_fd_intern(epl, el_payload->fd, event);
}

void crtx_epoll_mod_fd(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload) {
	crtx_epoll_mod_fd_intern((struct crtx_epoll_listener *) lbase, el_payload->fd, el_payload->el_data);
}

void crtx_epoll_del_fd(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload) {
	struct crtx_epoll_listener *epl;
	int fd;
	
	
	printf("epoll del %d\n", el_payload->fd);
	
	fd = el_payload->fd;
	el_payload->fd = 0;
	
	epl = (struct crtx_epoll_listener *) lbase;
	crtx_epoll_del_fd_intern(epl, fd);
	
	free(el_payload->el_data);
}

struct epoll_control_pipe {
	struct crtx_graph *graph;
};

void *crtx_epoll_main(void *data) {
	struct crtx_epoll_listener *epl;
	size_t i;
	int n_rdy_events;
	struct epoll_event *rec_events;
	struct crtx_event_loop_payload *el_payload;
	struct crtx_event *event;
	struct epoll_control_pipe ecp;
	
// 	printf("epoll main %p\n", data);
	epl = (struct crtx_epoll_listener*) data;
	
// 	for (i=0; i < epl->n_events; i++) {
// 		s = epoll_ctl(epl->epoll_fd, EPOLL_CTL_ADD, epl->fds[i].data.fd, &epl->fds[i].data);
// 		if (s < 0) {
// 			ERROR("epoll_ctl failed for fd %d: %s\n", event->data.fd, strerror(errno));
// 			continue;
// 		}
// 	}
	
	
	
	rec_events = (struct epoll_event*) malloc(sizeof(struct epoll_event)*epl->max_n_events);
	
	while (!epl->stop) {
		n_rdy_events = epoll_wait(epl->epoll_fd, rec_events, epl->max_n_events, epl->timeout);
		if (n_rdy_events < 0 && errno == EINTR)
			continue;
		
		if (n_rdy_events < 0) {
			ERROR("epoll_wait failed: %s\n", strerror(errno));
			break;
		}
		
		DBG("epoll returned with %d events\n", n_rdy_events);
		
		for (i=0; i < n_rdy_events; i++) {
			if (rec_events[i].events & EPOLLERR) {
				el_payload = (struct crtx_event_loop_payload* ) rec_events[i].data.ptr;
				
				ERROR("epoll returned EPOLLERR for fd %d\n", el_payload->fd);
				
				el_payload->error_cb(el_payload, el_payload->error_cb_data);
				
// 				crtx_epoll_del_fd((struct crtx_listener_base *) epl, el_payload);
				
				continue;
			} else
			if (rec_events[i].data.ptr == 0) {
				DBG("epoll received wake-up event\n");
				
				read(epl->pipe_fds[0], &ecp, sizeof(struct epoll_control_pipe));
				
				if (ecp.graph)
					crtx_process_graph_tmain(ecp.graph);
			} else {
				el_payload = (struct crtx_event_loop_payload* ) rec_events[i].data.ptr;
				
				event = create_event(0, 0, 0);
				
				memcpy(el_payload->el_data, &rec_events[i], sizeof(struct epoll_event));
				
				if (el_payload->event_handler) {
					event->data.raw.pointer = el_payload;
					event->data.raw.type = 'p';
					event->data.raw.flags = DIF_DATA_UNALLOCATED;
					
					// TODO we call the event handler directly as walking through the event graph
					// only causes additional processing overhead in most cases
					el_payload->event_handler(event, 0, 0);
					free_event(event);
					
					
					// TODO if this is called, either create individual graph per fd or ensure
					// the event handlers check if the event is meant for them
// 					add_event(epl->parent.graph, event);
				} else
				if (el_payload->simple_callback) {
					el_payload->simple_callback(el_payload);
				}
			}
		}
		
// 		if (epl->process_events) {
// 			struct crtx_dll *git, *eit;
// 			
// 			crtx_claim_next_event(&git, &eit);
// 			
// 			crtx_process_event(git->graph, eit);
// 		}
	}
	
	DBG("epoll stops\n");
	
	free(rec_events);
	
	return 0;
}

void crtx_epoll_queue_graph(struct crtx_epoll_listener *epl, struct crtx_graph *graph) {
	struct epoll_control_pipe ecp;
	
	ecp.graph = graph;
	write(epl->pipe_fds[1], &ecp, sizeof(struct epoll_control_pipe));
}

void crtx_epoll_stop(struct crtx_epoll_listener *epl) {
	struct epoll_control_pipe ecp;
	
	epl->stop = 1;
	
	ecp.graph = 0;
	write(epl->pipe_fds[1], &ecp, sizeof(struct epoll_control_pipe));
}

static void stop_thread(struct crtx_thread *t, void *data) {
	struct crtx_epoll_listener *epl;
	
	epl = (struct crtx_epoll_listener*) data;
	
	crtx_epoll_stop(epl);
}

static void shutdown_epoll_listener(struct crtx_listener_base *lbase) {
	struct crtx_epoll_listener *epl;
	
	epl = (struct crtx_epoll_listener*) lbase;
	
// 	stop_thread(0, lbase);
	
	close(epl->pipe_fds[1]);
	close(epl->pipe_fds[0]);
	close(epl->epoll_fd);
}

struct crtx_listener_base *crtx_new_epoll_listener(void *options) {
	struct crtx_epoll_listener *epl;
// 	int pipe_fds[2];
	struct epoll_event ctrl_event;
	int ret;
	
	
	epl = (struct crtx_epoll_listener*) options;
	
	epl->epoll_fd = epoll_create1(0);
	if (epl->epoll_fd < 0) {
		ERROR("epoll_create1 failed: %s\n", strerror(errno));
		return 0;
	}
	
	new_eventgraph(&epl->parent.graph, "epoll", 0);
	
	epl->parent.shutdown = &shutdown_epoll_listener;
	
// 	if (!epl->no_thread) {
		epl->parent.thread = get_thread(crtx_epoll_main, epl, 0);
		epl->parent.thread->do_stop = &stop_thread;
// 	} 
// 	else {
// 		if (crtx_root->event_loop.listener)
// 			ERROR("multiple crtx_root->event_loop\n");
// 		
// 		crtx_root->event_loop.listener = epl;
// 	}
	
	ret = pipe(epl->pipe_fds); // 0 read, 1 write
	if (ret < 0) {
		ERROR("creating control pipe failed: %s\n", strerror(errno));
// 		epl->control_fd = -1;
	} else {
// 		epl->control_fd = pipe_fds[1];
		
		memset(&ctrl_event, 0, sizeof(struct epoll_event));
		ctrl_event.events = EPOLLIN;
		ctrl_event.data.ptr = 0;
		
		crtx_epoll_add_fd_intern(epl, epl->pipe_fds[0], &ctrl_event);
	}
	
	if (epl->max_n_events < 1)
		epl->max_n_events = 1;
	
	// 0 is our default, -1 is no timeout for epoll
	epl->timeout -= 1;
	
	return &epl->parent;
}

void crtx_epoll_init() {
}

void crtx_epoll_finish() {
}


#ifdef CRTX_TEST
#include <fcntl.h>
#include "timer.h"

struct crtx_timer_listener tlist;
struct itimerspec newtimer;
struct crtx_listener_base *blist;
int testpipe[2];
int count = 0;

static char epoll_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	char buf[1024];
	struct crtx_event_loop_payload *payload;
	ssize_t n_read;
	
	payload = (struct crtx_event_loop_payload*) event->data.raw.pointer;
	
	n_read = read(payload->fd, buf, sizeof(buf)-1);
	if (n_read < 0) {
		ERROR("epoll test read failed: %s\n", strerror(errno));
		return 1;
	}
	buf[n_read] = 0;
	
	printf("read: \"%s\" (%zd)\n", buf, n_read);
	
	return 1;
}

static char timer_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	if (count > 2) {
		crtx_init_shutdown();
	} else {
		write(testpipe[1], "success", 7);
		count++;
	}
	
	return 1;
}

void start_timer() {
	// set time for (first) alarm
	tlist.newtimer.it_value.tv_sec = 1;
	tlist.newtimer.it_value.tv_nsec = 0;
	
	// set interval for repeating alarm, set to 0 to disable repetition
	tlist.newtimer.it_interval.tv_sec = 10;
	tlist.newtimer.it_interval.tv_nsec = 0;
	
	tlist.clockid = CLOCK_REALTIME; // clock source, see: man clock_gettime()
	tlist.settime_flags = 0; // absolute (TFD_TIMER_ABSTIME), or relative (0) time, see: man timerfd_settime()
// 	tlist.newtimer = &newtimer;
	
	blist = create_listener("timer", &tlist);
	if (!blist) {
		ERROR("create_listener(timer) failed\n");
		exit(1);
	}
	
	crtx_create_task(blist->graph, 0, "timer", timer_handler, 0);
	
	crtx_start_listener(blist);
}

int epoll_main(int argc, char **argv) {
	struct crtx_epoll_listener epl;
	struct crtx_listener_base *lbase;
	char ret;
	struct epoll_event event;
	struct crtx_event_loop_payload payload;
	
	memset(&epl, 0, sizeof(struct crtx_epoll_listener));
	
	if (argc > 1) {
// 		epl.no_thread = 1;
		crtx_root->force_mode = CRTX_PREFER_ELOOP;
	}
	
	lbase = create_listener("epoll", &epl);
	if (!lbase) {
		ERROR("create_listener(epoll) failed\n");
		exit(1);
	}
	
	ret = pipe(testpipe); // 0 read, 1 write
	if (ret < 0) {
		ERROR("creating test pipe failed: %s\n", strerror(errno));
		return 1;
	}
	
	memset(&payload, 0, sizeof(struct crtx_event_loop_payload));
	payload.fd = testpipe[0];
	
	event.events = EPOLLIN;
// 	event.data.fd = fd;
	event.data.ptr = &payload;
	crtx_epoll_add_fd_intern(&epl, testpipe[0], &event);
	
	crtx_create_task(lbase->graph, 0, "epoll_test", epoll_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting epoll listener failed\n");
		return 1;
	}
	
	if (argc > 1) {
		start_timer();
		
		crtx_loop();
		
		free_listener(blist);
	} else {
		struct epoll_control_pipe ecp;
		
		sleep(1);
		write(testpipe[1], "success", 7);
		
		sleep(1);
		
		ecp.graph = 0;
		write(epl.pipe_fds[1], &ecp, sizeof(struct epoll_control_pipe));
	}
	
	free_listener(lbase);
	
	close(testpipe[0]);
	
	return 0;
}

CRTX_TEST_MAIN(epoll_main);
#endif
