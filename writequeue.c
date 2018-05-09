/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "intern.h"
#include "core.h"
#include "epoll.h"
#include "writequeue.h"

static char fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *payload;
	struct crtx_writequeue *wqueue;
	struct crtx_event_loop *eloop;
	int r;
	
	payload = (struct crtx_event_loop_payload*) event->data.pointer;
	
	wqueue = (struct crtx_writequeue *) payload->data;
	
	r = wqueue->write(wqueue, wqueue->write_userdata);
	if (r != EAGAIN && r != ENOBUFS) {
		printf("write done %s %d\n", strerror(r), r);
		wqueue->parent.el_payload.event_flags = 0;
		
		eloop = crtx_get_event_loop();
		crtx_epoll_mod_fd(&eloop->listener->parent, &wqueue->parent.el_payload);
	}
	
	return 0;
}

void crtx_writequeue_start(struct crtx_writequeue *wqueue) {
	struct crtx_event_loop *eloop;
	
	wqueue->parent.el_payload.event_flags = EPOLLOUT;
	
	eloop = crtx_get_event_loop();
	crtx_epoll_mod_fd(&eloop->listener->parent, &wqueue->parent.el_payload);
}

void crtx_writequeue_stop(struct crtx_writequeue *wqueue) {
	struct crtx_event_loop *eloop;
	
	wqueue->parent.el_payload.event_flags = 0;
	
	eloop = crtx_get_event_loop();
	crtx_epoll_mod_fd(&eloop->listener->parent, &wqueue->parent.el_payload);
}

int crtx_add_writequeue2listener(struct crtx_writequeue *writequeue, struct crtx_listener_base *listener, crtx_wq_write_cb write_cb, void *write_userdata) {
	struct crtx_listener_base *lbase;
	
	memset(writequeue, 0, sizeof(struct crtx_writequeue));
	
	listener->el_payload.epollout = &writequeue->parent.el_payload;
	writequeue->parent.el_payload.epollin = &listener->el_payload;
	
	writequeue->write_fd = listener->el_payload.fd;
	writequeue->write = write_cb;
	writequeue->write_userdata = write_userdata;
	
	writequeue->parent.el_payload.el_data = (struct epoll_event*) calloc(1, sizeof(struct epoll_event));
	((struct epoll_event*) writequeue->parent.el_payload.el_data)->data.ptr = &writequeue->parent.el_payload;
	
	lbase = create_listener("writequeue", writequeue);
	if (!lbase) {
		ERROR("create_listener(writequeue) failed\n");
		exit(1);
	}
	
	return 0;
}

struct crtx_listener_base *crtx_new_writequeue_listener(void *options) {
	struct crtx_writequeue *wqueue;
// 	struct epoll_event ctrl_event;
	
	
	wqueue = (struct crtx_writequeue *) options;
	
	wqueue->parent.el_payload.fd = wqueue->write_fd;
	wqueue->parent.el_payload.event_flags = 0;
	wqueue->parent.el_payload.data = wqueue;
	wqueue->parent.el_payload.event_handler = fd_event_handler;
	
// 	wqueue->parent.shutdown = &crtx_sdbus_shutdown_listener;
	
	return &wqueue->parent;
}

void crtx_writequeue_init() {
}

void crtx_writequeue_finish() {
}

#ifdef CRTX_TEST

#include <unistd.h>
#include <fcntl.h>

int pipe_fds[2];
char stop;
#define BUF_SIZE 1024
char buffer[BUF_SIZE];

static int wq_write(struct crtx_writequeue *wqueue, void *userdata) {
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
	struct crtx_writequeue wqueue;
	struct crtx_listener_base *lbase;
	char ret;
	
	
	memset(&wqueue, 0, sizeof(struct crtx_writequeue));
	
	ret = pipe2(pipe_fds, O_NONBLOCK); // 0 read, 1 write
	if (ret < 0) {
		ERROR("creating control pipe failed: %s\n", strerror(errno));
	}
	
	wqueue.write_fd = pipe_fds[1];
	wqueue.write = &wq_write;
	
	lbase = create_listener("writequeue", &wqueue);
	if (!lbase) {
		ERROR("create_listener(writequeue) failed\n");
		exit(1);
	}
	
	ret = crtx_start_listener(lbase);
	if (ret) {
		ERROR("starting writequeue listener failed\n");
		return 1;
	}
	
	stop = 0;
	
	wq_write(&wqueue, 0);
	
	crtx_loop();
	
	return 0;  
}

CRTX_TEST_MAIN(writequeue_main);

#endif
