#ifndef _CRTX_PIPE_H
#define _CRTX_PIPE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2018
 *
 */

#define CRTX_READ_END (0)
#define CRTX_WRITE_END (1)

struct crtx_pipe_listener {
	struct crtx_listener_base base;
	
	int fds[2];
	char *event_type;
	
	crtx_handle_task_t fd_event_handler;
	void *fd_event_handler_data;
};

struct crtx_listener_base *crtx_setup_pipe_listener(void *options);
void crtx_pipe_clear_lstnr(struct crtx_pipe_listener *lstnr);
CRTX_DECLARE_ALLOC_FUNCTION(pipe)

void crtx_pipe_init();
void crtx_pipe_finish();

#ifdef __cplusplus
}
#endif

#endif
