
#ifndef _CRTX_EPOLL_H
#define _CRTX_EPOLL_H

// #include <sys/epoll.h>

// struct crtx_epoll_event_payload {
// 	int fd;
// 	struct epoll_event event;
// 	
// 	void *data;
// };

struct crtx_epoll_listener {
	struct crtx_listener_base parent;
	
	int epoll_fd;
	int control_fd;
	
	size_t max_n_events;
	int timeout;
	
// 	struct epoll_event *fds; // malloc'ed memory
// 	size_t n_fds;
	
	char no_thread;
// 	char process_events;
	char stop;
	
// 	int socket_protocol;
// 	
// 	sa_family_t nl_family;
// 	// 	unsigned short nl_pad;
// 	// 	pid_t nl_pid;
// 	uint32_t nl_groups;
// 	
// 	uint16_t *nlmsg_types;
// 	
// 	struct crtx_dict *(*raw2dict)(struct crtx_epoll_listener *nl_listener, struct nlmsghdr *nlh);
// 	char all_fields;
// 	
// 	struct crtx_signal msg_done;
// 	
// 	int sockfd;
// 	char stop;
};

void *crtx_epoll_main(void *data);
void crtx_epoll_stop(struct crtx_epoll_listener *epl);
struct crtx_listener_base *crtx_new_epoll_listener(void *options);
void crtx_epoll_init();
void crtx_epoll_finish();

// struct crtx_event_loop_payload * crtx_epoll_add_fd(struct crtx_listener_base *lbase, int fd, void *data, char *event_handler_name, crtx_handle_task_t event_handler);
void crtx_epoll_add_fd(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload);
void crtx_epoll_del_fd(struct crtx_listener_base *lbase, struct crtx_event_loop_payload *el_payload);
void crtx_epoll_queue_graph(struct crtx_epoll_listener *epl, struct crtx_graph *graph);

#endif
