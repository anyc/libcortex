
struct crtx_epoll_event_payload {
	struct epoll_event *event;
	
	struct crtx_graph *graph;
	void *data;
};

struct crtx_epoll_listener {
	struct crtx_listener_base parent;
	
	int epoll_fd;
	int control_fd;
	
	size_t max_n_events;
	int timeout;
	
// 	struct epoll_event *fds; // malloc'ed memory
// 	size_t n_fds;
	
	char process_events;
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

struct crtx_listener_base *crtx_new_epoll_listener(void *options);
void crtx_epoll_init();
void crtx_epoll_finish();