
#ifndef _CRTX_EPOLL_H
#define _CRTX_EPOLL_H

struct crtx_epoll_listener {
	struct crtx_listener_base parent;
	
	int epoll_fd;
// 	int control_fd;
	int pipe_fds[2];
	
	size_t max_n_events;
	int timeout;
	
// 	char no_thread;
	char stop;
};

void *crtx_epoll_main(void *data);
void crtx_epoll_stop(struct crtx_epoll_listener *epl);
struct crtx_listener_base *crtx_new_epoll_listener(void *options);
void crtx_epoll_init();
void crtx_epoll_finish();


char *epoll_flags2str(int *flags);
// struct crtx_event_loop_payload * crtx_epoll_add_fd(struct crtx_listener_base *lbase, int fd, void *data, char *event_handler_name, crtx_handle_task_t event_handler);
void crtx_epoll_add_fd(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload);
void crtx_epoll_mod_fd(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload);
void crtx_epoll_del_fd(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload);
void crtx_epoll_queue_graph(struct crtx_epoll_listener *epl, struct crtx_graph *graph);

#endif
