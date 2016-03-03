
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "core.h"
#include "epoll.h"

int crtx_epoll_add_fd(struct crtx_epoll_listener *epl, struct epoll_event *event) {
	int ret;
	
	event->events |= EPOLLET;
	
// 	s = epoll_ctl(epl->epoll_fd, EPOLL_CTL_ADD, epl->fds[i].data.fd, &epl->fds[i].data);
	ret = epoll_ctl(epl->epoll_fd, EPOLL_CTL_ADD, event->data.fd, event);
	if (ret < 0) {
		ERROR("epoll_ctl failed for fd %d: %s\n", event->data.fd, strerror(errno));
		
		return 0;
	}
	
	return 1;
}

static void *epoll_tmain(void *data) {
	struct crtx_epoll_listener *epl;
	size_t i;
	int n_rdy_events;
	int pipe_fds[2];
	int ret;
	struct epoll_event ctrl_event;
	struct epoll_event *rec_events;
	
	struct crtx_epoll_event_payload *ev_payload;
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
		ctrl_event.data.fd = pipe_fds[0];
		
		crtx_epoll_add_fd(epl, &ctrl_event);
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
				ERROR("EPOLLERR for fd %d\n", rec_events[i].data.fd);
				continue;
			} else
			if (rec_events[i].data.fd == pipe_fds[0]) {
				DBG("epoll received wake-up event\n");
				
				read(rec_events[i].data.fd, &ret, 1);
				
				continue;
			} else {
				ev_payload = (struct crtx_epoll_event_payload* ) rec_events[i].data.ptr;
				
				event = create_event(0, 0, 0);
				
				event->data.raw.pointer = ev_payload;
				event->data.raw.type = 'p';
				event->data.raw.flags = DIF_DATA_UNALLOCATED;
				
				add_event(ev_payload->graph, event);
			}
		}
		
		if (epl->process_events) {
			struct crtx_dll *git, *tit;
			
			for (git=crtx_root->graph_queue; git; git = git->next) {
				for (tit=git->graph->equeue; tit; tit = tit->next) {
					
				}
			}
		}
	}
	
	close(epl->control_fd);
	close(epl->epoll_fd);
	
	return 0;
}

void free_epoll_listener(struct crtx_listener_base *lbase) {
	struct crtx_epoll_listener *epl;
	
	epl = (struct crtx_epoll_listener *) lbase;
	
	epl->stop = 1;
	write(epl->control_fd, &epl, 1);
}

struct crtx_listener_base *crtx_new_epoll_listener(void *options) {
	struct crtx_epoll_listener *epl;
// 	struct sockaddr_nl addr;
	
	epl = (struct crtx_epoll_listener*) options;
	
	epl->epoll_fd = epoll_create1(0);
	if (epl->epoll_fd < 0) {
		ERROR("epoll_create1 failed: %s\n", strerror(errno));
		return 0;
	}
	
	new_eventgraph(&epl->parent.graph, 0, 0);
	
// 	init_signal(&nl_listener->msg_done);
	
	epl->parent.free = &free_epoll_listener;
	epl->parent.start_listener = 0;
	epl->parent.thread = get_thread(epoll_tmain, epl, 0);
// 	epl->parent.thread->do_stop = &stop_thread;
// 	reference_signal(&epl->parent.thread->finished);
	
	return &epl->parent;
}

void crtx_epoll_init() {
}

void crtx_epoll_finish() {
}


#ifdef CRTX_TEST
int epoll_main(int argc, char **argv) {
	struct crtx_epoll_listener epl;
	struct crtx_listener_base *lbase;
	char ret;
	
	memset(&epl, 0, sizeof(struct crtx_epoll_listener));
	
	lbase = create_listener("epoll", &epl);
	if (!lbase) {
		ERROR("create_listener(epoll) failed\n");
		exit(1);
	}

// 	crtx_create_task(lbase->graph, 0, "epoll_test", epoll_test_handler, 0);

	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting epoll listener failed\n");
		return 1;
	}
	
	sleep(5);
	write(epl.control_fd, &ret, 1);
	
	sleep(1);
	
	free_listener(lbase);
	
	return 0;
}

CRTX_TEST_MAIN(epoll_main);
#endif