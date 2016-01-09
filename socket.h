
#include <arpa/inet.h>

struct socket_listener {
	struct crtx_listener_base parent;
	
	char **recv_types;
	char **send_types;
	
	int ai_family; // AF_UNSPEC AF_INET AF_UNIX
	int type; // SOCK_STREAM
	int protocol; // IPPROTO_TCP
	
	char *host;
	char *service;
	
	int sockfd;
	int server_sockfd;
	
	struct crtx_graph *outbox;
	struct crtx_graph *crtx_inbox;
	struct crtx_graph *crtx_outbox;
	
	struct queue_event *sent_events;
	
	pthread_t thread;
	char stop;
};


void crtx_socket_init();
void crtx_socket_finish();
struct crtx_listener_base *crtx_new_socket_server_listener(void *options);
struct crtx_listener_base *crtx_new_socket_client_listener(void *options);

// void send_event(struct socket_listener *slistener, struct crtx_event *event);