#ifndef CRTX_WRITE_QUEUE_H
#define CRTX_WRITE_QUEUE_H

struct crtx_writequeue {
	struct crtx_listener_base parent;
	
	int write_fd;
	void *buffer;
	size_t buffer_size;
	
	// 	int pipe_fds[2];
	
	// 	crtx_handle_task_t fd_event_handler;
	int (*write)(struct crtx_writequeue *wqueue, void *userdata);
	void *write_userdata;
};

struct crtx_listener_base *crtx_writequeue_new_listener(void *options);
void crtx_writequeue_start(struct crtx_writequeue *wqueue);
void crtx_writequeue_stop(struct crtx_writequeue *wqueue);

#endif
