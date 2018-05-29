#ifndef _CRTX_SOCKET_RAW_H
#define _CRTX_SOCKET_RAW_H

#include <arpa/inet.h>

#ifndef CRTX_UNIX_SOCKET_DIR
#define CRTX_UNIX_SOCKET_DIR "/var/run/cortexd/"
#endif

#include <netinet/in.h>

struct crtx_socket_raw_listener {
	struct crtx_listener_base base;
	
	int ai_family; // AF_UNSPEC AF_INET AF_UNIX
	int type; // SOCK_STREAM
	int protocol; // IPPROTO_TCP
	
	char *host;
	char *service;
	
	int listen_backlog;
	socklen_t addrlen;
	
	int sockfd;
	int server_sockfd;
	
	void (*accept_cb)(struct crtx_socket_raw_listener *slist, int fd, struct sockaddr *cliaddr);
};

struct addrinfo *crtx_get_addrinfo(struct crtx_socket_raw_listener *listener);
void crtx_free_addrinfo(struct addrinfo *result);

void crtx_socket_raw_init();
void crtx_socket_raw_finish();
struct crtx_listener_base *crtx_new_socket_raw_listener(void *options);
struct crtx_listener_base *crtx_new_socket_raw_server_listener(void *options);
struct crtx_listener_base *crtx_new_socket_raw_client_listener(void *options);

#endif
