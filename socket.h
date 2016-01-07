
#include <arpa/inet.h>

struct socket_listener {
	struct listener parent;
	
	char **recv_types;
	char **send_types;
	
	int ai_family; // AF_UNSPEC AF_INET AF_UNIX
	int type; // SOCK_STREAM
	int protocol; // IPPROTO_TCP
	
	char *host;
	char *service;
	
	int sockfd;
	int server_sockfd;
	
	struct event_graph *outbox;
	struct event_graph *crtx_inbox;
	struct event_graph *crtx_outbox;
	
	struct queue_event *sent_events;
	
	pthread_t thread;
	char stop;
};


void socket_init();
void socket_finish();
struct listener *new_socket_server_listener(void *options);
struct listener *new_socket_client_listener(void *options);

// void send_event(struct socket_listener *slistener, struct event *event);