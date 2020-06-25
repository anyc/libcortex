#ifndef _CRTX_SOCKET_H
#define _CRTX_SOCKET_H

#include <arpa/inet.h>

#ifndef CRTX_UNIX_SOCKET_DIR
#define CRTX_UNIX_SOCKET_DIR "/var/run/cortexd/"
#endif

struct crtx_socket_listener {
	struct crtx_listener_base base;
	
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
	
	char stop;
};


struct crtx_listener_base *crtx_setup_socket_server_listener(void *options);
struct crtx_listener_base *crtx_setup_socket_client_listener(void *options);

CRTX_DECLARE_ALLOC_FUNCTION(socket)

void crtx_socket_init();
void crtx_socket_finish();

#endif
