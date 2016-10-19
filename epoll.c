
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
	
	event->events |= EPOLLET;
	
	ret = epoll_ctl(epl->epoll_fd, EPOLL_CTL_ADD, fd, event);
	if (ret < 0) {
		ERROR("epoll_ctl failed for fd %d: %s\n", fd, strerror(errno));
		
		return 0;
	}
	
	return 1;
}

void crtx_epoll_add_fd(struct crtx_listener_base *lbase, int fd, void *data, char *event_handler_name, crtx_handle_task_t event_handler) {
	struct crtx_event_loop_payload *payload;
	struct epoll_event *event;
	struct crtx_epoll_listener *epl;
	
	
	payload = (struct crtx_event_loop_payload*) calloc(1, sizeof(struct crtx_event_loop_payload));
	payload->fd = fd;
	payload->data = data;
	
	event = (struct epoll_event*) calloc(1, sizeof(struct epoll_event));
	event->events = EPOLLIN;
	event->data.ptr = payload;
	
	epl = (struct crtx_epoll_listener *) lbase;
// 	printf("lis %d\n", epl->epoll_fd);
	
	crtx_create_task(lbase->graph, 0, event_handler_name, event_handler, 0);
	
	crtx_epoll_add_fd_intern(epl, payload->fd, event);
}

void *crtx_epoll_main(void *data) {
	struct crtx_epoll_listener *epl;
	size_t i;
	int n_rdy_events;
	int pipe_fds[2];
	int ret;
	struct epoll_event ctrl_event;
	struct epoll_event *rec_events;
	
	struct crtx_event_loop_payload *ev_payload;
	struct crtx_event *event;
	
	epl = (struct crtx_epoll_listener*) data;
	
// 	for (i=0; i < epl->n_events; i++) {
// 		s = epoll_ctl(epl->epoll_fd, EPOLL_CTL_ADD, epl->fds[i].data.fd, &epl->fds[i].data);
// 		if (s < 0) {
// 			ERROR("epoll_ctl failed for fd %d: %s\n", event->data.fd, strerror(errno));
// 			continue;
// 		}
// 	}
	
	ret = pipe(pipe_fds); // 0 read, 1 write
	if (ret < 0) {
		ERROR("creating control pipe failed: %s\n", strerror(errno));
		epl->control_fd = -1;
	} else {
		epl->control_fd = pipe_fds[1];
		
		ctrl_event.events = EPOLLIN;
		ctrl_event.data.ptr = 0;
		
		crtx_epoll_add_fd_intern(epl, pipe_fds[0], &ctrl_event);
	}
	
	if (epl->max_n_events < 1)
		epl->max_n_events = 1;
	
	// 0 is our default, -1 is no timeout for epoll
	epl->timeout -= 1;
	
	rec_events = (struct epoll_event*) malloc(sizeof(struct epoll_event)*epl->max_n_events);
	
	while (!epl->stop) {
		n_rdy_events = epoll_wait(epl->epoll_fd, rec_events, epl->max_n_events, epl->timeout);
		if (n_rdy_events < 0) {
			ERROR("epoll_wait failed: %s\n", strerror(errno));
			break;
		}
		
		DBG("epoll returned with %d events\n", n_rdy_events);
		
		for (i=0; i < n_rdy_events; i++) {
			if (rec_events[i].events & EPOLLERR) {
				ERROR("epoll returned EPOLLERR\n");
				continue;
			} else
			if (rec_events[i].data.ptr == 0) {
				DBG("epoll received wake-up event\n");
				
				read(pipe_fds[0], &ret, 1);
				
				continue;
			} else {
				ev_payload = (struct crtx_event_loop_payload* ) rec_events[i].data.ptr;
				
				event = create_event(0, 0, 0);
				
				memcpy(&ev_payload->event, &rec_events[i], sizeof(struct epoll_event));
				event->data.raw.pointer = ev_payload;
				event->data.raw.type = 'p';
				event->data.raw.flags = DIF_DATA_UNALLOCATED;
				
				add_event(epl->parent.graph, event);
			}
		}
		
		if (epl->process_events) {
			struct crtx_dll *git, *eit;
			
			crtx_claim_next_event(&git, &eit);
			
			crtx_process_event(git->graph, eit);
		}
	}
	
	DBG("epoll stops\n");
	
	close(epl->control_fd);
	close(epl->epoll_fd);
	
	return 0;
}

void crtx_epoll_stop(struct crtx_epoll_listener *epl) {
	epl->stop = 1;
	write(epl->control_fd, &epl, 1);
}

static void stop_thread(struct crtx_thread *t, void *data) {
	struct crtx_epoll_listener *epl;
	
	epl = (struct crtx_epoll_listener*) data;
	
	crtx_epoll_stop(epl);
}

static void free_epoll_listener(struct crtx_listener_base *lbase) {
	stop_thread(0, lbase);
}

struct crtx_listener_base *crtx_new_epoll_listener(void *options) {
	struct crtx_epoll_listener *epl;
	
	epl = (struct crtx_epoll_listener*) options;
	
	epl->epoll_fd = epoll_create1(0);
	if (epl->epoll_fd < 0) {
		ERROR("epoll_create1 failed: %s\n", strerror(errno));
		return 0;
	}
	
	new_eventgraph(&epl->parent.graph, 0, 0);
	
	epl->parent.free = &free_epoll_listener;
	epl->parent.start_listener = 0;
	if (!epl->no_thread) {
		epl->parent.thread = get_thread(crtx_epoll_main, epl, 0);
		epl->parent.thread->do_stop = &stop_thread;
	} else {
		if (crtx_root->event_loop.listener)
			ERROR("multiple crtx_root->event_loop\n");
		
		crtx_root->event_loop.listener = epl;
	}
	
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
	newtimer.it_value.tv_sec = 1;
	newtimer.it_value.tv_nsec = 0;
	
	// set interval for repeating alarm, set to 0 to disable repetition
	newtimer.it_interval.tv_sec = 10;
	newtimer.it_interval.tv_nsec = 0;
	
	tlist.clockid = CLOCK_REALTIME; // clock source, see: man clock_gettime()
	tlist.settime_flags = 0; // absolute (TFD_TIMER_ABSTIME), or relative (0) time, see: man timerfd_settime()
	tlist.newtimer = &newtimer;
	
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
		epl.no_thread = 1;
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
		sleep(1);
		write(testpipe[1], "success", 7);
		
		sleep(1);
		write(epl.control_fd, &ret, 1);
	}
	
	free_listener(lbase);
	
	close(testpipe[0]);
	
	return 0;
}

CRTX_TEST_MAIN(epoll_main);
#endif
