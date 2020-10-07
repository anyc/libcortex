#ifndef CRTX_WRITE_QUEUE_H
#define CRTX_WRITE_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

struct crtx_writequeue_listener;
typedef int (*crtx_wq_write_cb)(struct crtx_writequeue_listener *wqueue, void *userdata);

struct crtx_writequeue_listener {
	struct crtx_listener_base base;
// 	struct epoll_event epoll_event;
	
	struct crtx_listener_base *read_listener;
	
	struct crtx_evloop_callback evloop_cb;
	
	int write_fd;
	
	crtx_wq_write_cb write;
	void *write_userdata;
};

int crtx_add_writequeue2listener(struct crtx_writequeue_listener *writequeue, struct crtx_listener_base *listener, crtx_wq_write_cb write_cb, void *write_userdata);

struct crtx_listener_base *crtx_setup_writequeue_listener(void *options);
void crtx_writequeue_start(struct crtx_writequeue_listener *wqueue);
void crtx_writequeue_stop(struct crtx_writequeue_listener *wqueue);
CRTX_DECLARE_ALLOC_FUNCTION(writequeue)

void crtx_writequeue_init();
void crtx_writequeue_finish();

#ifdef __cplusplus
}
#endif

#endif
