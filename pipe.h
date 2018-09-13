#ifndef _CRTX_PIPE_H
#define _CRTX_PIPE_H

/*
 * Mario Kicherer (dev@kicherer.org) 2018
 *
 */

struct crtx_pipe_listener {
	struct crtx_listener_base base;
	
	int fds[2];
	char *event_type;
	
	crtx_handle_task_t fd_event_handler;
	void *fd_event_handler_data;
};

struct crtx_listener_base *crtx_new_pipe_listener(void *options);

void crtx_pipe_init();
void crtx_pipe_finish();

#endif
