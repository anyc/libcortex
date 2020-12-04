/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#ifdef CRTX_TEST
// for pipe2()
#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "intern.h"
#include "core.h"
#include "epoll.h"
#include "writequeue.h"

static char fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct crtx_evloop_fd *payload;
	struct crtx_writequeue_listener *wqueue;
// 	struct crtx_event_loop *eloop;
	int r;
// 	struct crtx_evloop_callback *el_cb;
	
// 	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
// 	payload = (struct crtx_evloop_fd*) event->data.pointer;
	
	wqueue = (struct crtx_writequeue_listener *) userdata;
	
	r = wqueue->write(wqueue, wqueue->write_userdata);
	if (r != EAGAIN && r != ENOBUFS) {
		CRTX_VDBG("write done %s %d\n", strerror(r), r);
// 		wqueue->base.evloop_fd.crtx_event_flags = 0;
// 		
// 		eloop = crtx_get_event_loop();
// 		crtx_epoll_mod_fd(&eloop->listener->base, &wqueue->base.evloop_fd);
		crtx_writequeue_stop(wqueue);
	}
	
	return 0;
}

void crtx_writequeue_start(struct crtx_writequeue_listener *wqueue) {
// // 	struct crtx_event_loop *eloop;
// 	
// // 	wqueue->base.evloop_fd.crtx_event_flags = EVLOOP_WRITE;
// 	wqueue->evloop_cb->active = 1;
// 	
// // 	eloop = crtx_get_event_loop();
// // 	crtx_epoll_mod_fd(&eloop->listener->base, &wqueue->base.evloop_fd);
// 	crtx_root->event_loop.mod_fd(&crtx_root->event_loop, &wqueue->base.evloop_fd);
	
	if (wqueue->read_listener)
		wqueue->evloop_cb.fd_entry->fd = wqueue->read_listener->evloop_fd.fd;
	
	crtx_evloop_enable_cb(&wqueue->evloop_cb);
}

void crtx_writequeue_stop(struct crtx_writequeue_listener *wqueue) {
// // 	struct crtx_event_loop *eloop;
// 	
// // 	wqueue->base.evloop_fd.crtx_event_flags = 0;
// 	wqueue->evloop_cb->active = 0;
// 	
// // 	eloop = crtx_get_event_loop();
// // 	crtx_epoll_mod_fd(&eloop->listener->base, &wqueue->base.evloop_fd);
// 	crtx_root->event_loop.mod_fd(&crtx_root->event_loop, &wqueue->base.evloop_fd);
	
	crtx_evloop_disable_cb(&wqueue->evloop_cb);
}

struct crtx_listener_base *crtx_setup_writequeue_listener(void *options) {
	struct crtx_writequeue_listener *wqueue;
	
	
	wqueue = (struct crtx_writequeue_listener *) options;
	
// 	wqueue->base.evloop_fd.fd = wqueue->write_fd;
// 	wqueue->base.evloop_fd.crtx_event_flags = 0;
// 	wqueue->base.evloop_fd.data = wqueue;
// 	wqueue->base.evloop_fd.event_handler = fd_event_handler;
	
	if (wqueue->read_listener) {
		crtx_evloop_create_fd_entry(&wqueue->read_listener->evloop_fd, &wqueue->evloop_cb,
							wqueue->write_fd,
							EVLOOP_WRITE,
							0,
							&fd_event_handler,
							wqueue,
							0, 0
						);
	} else {
		crtx_evloop_init_listener(&wqueue->base,
							wqueue->write_fd,
							EVLOOP_WRITE,
							0,
							&fd_event_handler, wqueue,
							0, 0
			);
	}
	
// 	wqueue->evloop_cb->active = 0;
	
	return &wqueue->base;
}

int crtx_add_writequeue2listener(struct crtx_writequeue_listener *writequeue, struct crtx_listener_base *listener, crtx_wq_write_cb write_cb, void *write_userdata) {
	int r;
	
	
	memset(writequeue, 0, sizeof(struct crtx_writequeue_listener));
	
// 	listener->evloop_fd.epollout = &writequeue->base.evloop_fd;
// 	writequeue->base.evloop_fd.epollin = &listener->evloop_fd;
	
	writequeue->write_fd = listener->evloop_fd.fd;
	writequeue->write = write_cb;
	writequeue->write_userdata = write_userdata;
	
// 	writequeue->base.evloop_fd.parent = &listener->evloop_fd;
	writequeue->read_listener = listener;
	
// 	crtx_ll_append(&listener->evloop_fd.sub_payloads, (struct crtx_ll*) &writequeue->base.evloop_fd);
	
// 	writequeue->base.evloop_fd.el_data = (struct epoll_event*) calloc(1, sizeof(struct epoll_event));
// 	((struct epoll_event*) writequeue->base.evloop_fd.el_data)->data.ptr = &writequeue->base.evloop_fd;

// 	writequeue->base.evloop_fd.el_data = &writequeue->epoll_event;
// 	writequeue->epoll_event.data.ptr = &writequeue->base.evloop_fd;
	
	r = crtx_setup_listener("writequeue", writequeue);
	if (r) {
		CRTX_ERROR("create_listener(writequeue) failed: %s\n", strerror(-r));
		return r;
	}
	
	return 0;
}

CRTX_DEFINE_ALLOC_FUNCTION(writequeue)

void crtx_writequeue_init() {
}

void crtx_writequeue_finish() {
}

#ifdef CRTX_TEST

int pipe_fds[2];
char stop;
#define BUF_SIZE 1024
char buffer[BUF_SIZE];

static int wq_write(struct crtx_writequeue_listener *wqueue, void *userdata) {
	ssize_t n_bytes;
	
	if (stop > 1)
		exit(0);
	
	while (1) {
		n_bytes = write(wqueue->write_fd, buffer, BUF_SIZE);
		if (n_bytes < 0) {
			if (errno == EAGAIN || errno == ENOBUFS) {
				printf("received EAGAIN\n");
				
				crtx_writequeue_start(wqueue);
				
				n_bytes = 1;
				while (n_bytes > 0) {
					n_bytes = read(pipe_fds[0], buffer, BUF_SIZE);
				}
				stop += 1;
				
				printf("reading done\n");
				
				return EAGAIN;
			} else {
				printf("error %s\n", strerror(errno));
				exit(1);
			}
		} else {
			printf(". %zu\n", n_bytes);
			fflush(stdin);
		}
	}
	
	return 0;
}

int writequeue_main(int argc, char **argv) {
	struct crtx_writequeue_listener wqueue;
	char ret;
	
	
	memset(&wqueue, 0, sizeof(struct crtx_writequeue_listener));
	
	ret = pipe2(pipe_fds, O_NONBLOCK); // 0 read, 1 write
	if (ret < 0) {
		CRTX_ERROR("creating control pipe failed: %s\n", strerror(errno));
	}
	
	wqueue.write_fd = pipe_fds[1];
	wqueue.write = &wq_write;
	
	ret = crtx_setup_listener("writequeue", &wqueue);
	if (ret) {
		CRTX_ERROR("create_listener(writequeue) failed: %s\n", strerror(-ret));
		exit(1);
	}
	
	ret = crtx_start_listener(&wqueue.base);
	if (ret) {
		CRTX_ERROR("starting writequeue listener failed\n");
		return 1;
	}
	
	stop = 0;
	
	wq_write(&wqueue, 0);
	
	crtx_loop();
	
	return 0;  
}

CRTX_TEST_MAIN(writequeue_main);

#endif
