/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#ifdef CRTX_TEST
// for pipe2()
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "intern.h"
#include "core.h"
#include "epoll.h"
#include "writequeue.h"


int crtx_writequeue_default_write_callback(struct crtx_writequeue_listener *writequeue_lstnr, void *userdata) {
	struct crtx_ll **writequeue;
	struct crtx_ll *it, *tmp;
	ssize_t err;
	size_t c;
	
	writequeue = (struct crtx_ll **) userdata;
	
	
	// send all events in our write queue
	c = 0;
	errno = 0;
	it = *writequeue;
	while (it) {
		err = write(writequeue_lstnr->write_fd, it->payload, it->size);
		if (err < 0 || ((unsigned) err != it->size) ) {
			if (errno != EAGAIN && errno != ENOBUFS) {
				size_t i=0;
				for (tmp = it; tmp; tmp=tmp->next) {i+=1;}
				
				CRTX_ERROR("write error (%d): %s (%d, queue length: %zu)\n", writequeue_lstnr->write_fd, strerror(errno), err, i);
			} else {
				size_t i=0;
				for (tmp = it; tmp; tmp=tmp->next) {i+=1;}
				
				err = errno;
				
				CRTX_DBG("writequeue suspended (errno: %s, queue length: %zu)\n", (errno==EAGAIN)?"EAGAIN":"ENOBUFS", i);
			}
			break;
		}
		
		// we do not use crtx_ll_unlink() as this is a bit faster in this case
		tmp = it;
		it = it->next;
		free(tmp);
		
		c += 1;
	}
	
	*writequeue = it;
	
	// if errno != EAGAIN the writequeue will stop itself
	return err;
}

static char fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_writequeue_listener *wqueue;
	int r;
	
	wqueue = (struct crtx_writequeue_listener *) userdata;
	
	crtx_lock_listener(&wqueue->base);
	
	r = wqueue->write(wqueue, wqueue->write_userdata);
	if (r != EAGAIN && r != ENOBUFS) {
		CRTX_VDBG("write fd %d paused, error \"%s\" (%d)\n", wqueue->write_fd, strerror(r), r);
		crtx_writequeue_stop(wqueue);
	} else {
		if (!wqueue->base.default_el_cb.active) {
			crtx_writequeue_start(wqueue);
		}
	}
	
	crtx_unlock_listener(&wqueue->base);
	
	return 0;
}

// this function should be called if we want to be notified by the kernel when
// we can try again to submit data
void crtx_writequeue_start(struct crtx_writequeue_listener *wqueue) {
	crtx_evloop_enable_cb(&wqueue->base.default_el_cb);
}

void crtx_writequeue_stop(struct crtx_writequeue_listener *wqueue) {
	crtx_evloop_disable_cb(&wqueue->base.default_el_cb);
}

struct crtx_listener_base *crtx_setup_writequeue_listener(void *options) {
	struct crtx_writequeue_listener *wqueue;
	int r, flags;
	
	
	wqueue = (struct crtx_writequeue_listener *) options;
	
	flags = fcntl(wqueue->write_fd, F_GETFL, 0);
	r = fcntl(wqueue->write_fd, F_SETFL, flags | O_NONBLOCK);
	if (r != 0) {
		CRTX_ERROR("fcntl(%d, O_NONBLOCK) failed: %s\n", wqueue->write_fd, strerror(errno));
		return 0;
	}
	
	// if we are associated with a listener that will only read from the file descriptor
	// we have to add our crtx_evloop_callback structure to the crtx_evloop_fd structure
	// of the read_listener
	if (wqueue->read_listener) {
		crtx_evloop_create_fd_entry(&wqueue->read_listener->evloop_fd, &wqueue->base.default_el_cb,
							wqueue->write_fd,
							EVLOOP_WRITE,
							0,
							&fd_event_handler, wqueue,
							0, 0
						);
	} else {
		crtx_evloop_create_fd_entry(&wqueue->base.evloop_fd, &wqueue->base.default_el_cb,
							wqueue->write_fd,
							EVLOOP_WRITE,
							0,
							&fd_event_handler, wqueue,
							0, 0
			);
	}
	
	return &wqueue->base;
}

int crtx_add_writequeue2listener(struct crtx_writequeue_listener *writequeue, struct crtx_listener_base *listener, crtx_wq_write_cb write_cb, void *write_userdata) {
	int r;
	
	
	memset(writequeue, 0, sizeof(struct crtx_writequeue_listener));
	
	writequeue->write_fd = listener->evloop_fd.fd;
	writequeue->write = write_cb;
	writequeue->write_userdata = write_userdata;
	
	writequeue->read_listener = listener;
	
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
