

struct socket_listener {
	struct listener parent;
	
	int ai_family; // AF_UNSPEC AF_INET AF_UNIX
	int type; // SOCK_STREAM
	int protocol; // IPPROTO_TCP
	
	char *host;
	char *service;
	
	struct event_graph *graph;
	pthread_t thread;
	char stop;
};

struct socket_req {
	unsigned int type_length;
	unsigned int data_length;
	char *type;
	char *data;
};


void socket_init();
void socket_finish();
void socket_send(struct socket_req *req);
struct listener *new_socket_listener(void *options);