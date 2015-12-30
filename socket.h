
#include <arpa/inet.h>

struct socket_listener {
	struct listener parent;
	
	int ai_family; // AF_UNSPEC AF_INET AF_UNIX
	int type; // SOCK_STREAM
	int protocol; // IPPROTO_TCP
	
	char *host;
	char *service;
	
	int sockfd;
	
// 	struct event_graph *graph;
// 	struct event_graph *inbound;
// 	struct event_graph *outbound;
// 	void (*return_event)(struct event *event);
	struct event_graph *response_graph;
	
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
struct listener *new_socket_server_listener(void *options);
struct listener *new_socket_client_listener(void *options);

void send_event(struct socket_listener *slistener, struct event *event);