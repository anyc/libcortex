/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

/*
 * To open a TCPv4 connection with localhost:1234
 * 
 * sock_listener.ai_family = AF_INET; // or AF_UNSPEC
 * sock_listener.type = SOCK_STREAM;
 * sock_listener.protocol = IPPROTO_TCP;
 * sock_listener.host = "localhost";
 * sock_listener.service = "1234";
 * sock_list = create_listener("socket_client", &sock_listener);
 * 
 * To open a Unix socket /tmp/mysocket:
 * 
 * sock_listener.ai_family = AF_UNIX;
 * sock_listener.type = SOCK_STREAM;
 * sock_listener.service = "/tmp/mysocket";
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>

#include "intern.h"
#include "core.h"
#include "socket_raw.h"

struct addrinfo *crtx_get_addrinfo(struct crtx_socket_raw_listener *listener) {
	struct addrinfo hints, *result;
	int ret;
	
	if (listener->ai_family == AF_UNIX) {
		struct sockaddr_un *sun;
		
		result = (struct addrinfo *) calloc(1, sizeof(struct addrinfo) + sizeof(struct sockaddr_un));
		result->ai_family = AF_UNIX;
		result->ai_socktype = listener->type;
		result->ai_next = 0;
		
		result->ai_addrlen = sizeof(struct sockaddr_un);
		result->ai_addr = (void *) (result) + sizeof(struct addrinfo);
		
		sun = (struct sockaddr_un *) result->ai_addr;
		
		sun->sun_family = AF_UNIX;
		strncpy(sun->sun_path, listener->service, sizeof(sun->sun_path)-1);
	} else {
		bzero(&hints, sizeof(struct addrinfo));
		hints.ai_flags=AI_PASSIVE;
		hints.ai_family=listener->ai_family;
		hints.ai_socktype=listener->type;
		hints.ai_protocol=listener->protocol;
		
		// getattrinfo modifies the host variable for some reason
// 		char *test = crtx_stracpy(listener->host, 0);
		ret = getaddrinfo(listener->host, listener->service, &hints, &result);
		if (ret !=0) {
			printf("getaddrinfo error %s %s: %s\n", listener->host, listener->service, gai_strerror(ret));
			return 0;
		}
// 		free(test);
	}
	
	return result;
}

void crtx_free_addrinfo(struct addrinfo *result) {
	if (result->ai_family == AF_UNIX) {
		free(result);
	} else {
		freeaddrinfo(result);
	}
}

static char socket_raw_accept_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *el_payload;
	struct crtx_socket_raw_listener *slist;
	struct sockaddr *cliaddr;
	int fd;
	
	
	el_payload = (struct crtx_event_loop_payload*) event->data.pointer;
	
	slist = (struct crtx_socket_raw_listener *) el_payload->data;
	
	cliaddr = malloc(slist->addrlen);
	fd = accept(slist->sockfd, cliaddr, &slist->addrlen);
	if (slist->sockfd < 0) {
		ERROR("accept failed: %d\n", fd);
		return 0;
	}
	
	slist->accept_cb(slist, fd, cliaddr);
	
	return 0;
}

void shutdown_socket_raw_listener(struct crtx_listener_base *data) {
	struct crtx_socket_raw_listener *slist;
	
	slist = (struct crtx_socket_raw_listener*) data;
	
	close(slist->sockfd);
}

struct crtx_listener_base *crtx_new_socket_raw_server_listener(void *options) {
	struct crtx_socket_raw_listener *slistener;
	struct addrinfo *addrinfos, *rit;
	int ret;
	
	slistener = (struct crtx_socket_raw_listener *) options;
	
	
	
	addrinfos = crtx_get_addrinfo(slistener);
	
	for (rit = addrinfos; rit; rit = rit->ai_next) {
		slistener->sockfd = socket(rit->ai_family, rit->ai_socktype, rit->ai_protocol);
		
		if (slistener->sockfd < 0)
			continue;
		
		int enable = 1;
		ret = setsockopt(slistener->sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
		if (ret < 0) {
			ERROR("setsockopt(SO_REUSEADDR) failed");
		}
		
		if (rit->ai_family == AF_UNIX) {
			ret = unlink( ((struct sockaddr_un *)rit->ai_addr)->sun_path );
			if (ret < 0)
				ERROR("failed to unlink existing unix socket: %s\n", strerror(errno));
		}
		
		if (bind(slistener->sockfd, rit->ai_addr, rit->ai_addrlen) == 0)
			break;
		
		close(slistener->sockfd);
	}
	if (rit == 0) {
		ERROR("crtx_new_socket_raw_server_listener: %s\n", strerror(errno));
		return 0;
	}
	
	if (slistener->listen_backlog == 0)
		slistener->listen_backlog = 5;
	
	listen(slistener->sockfd, slistener->listen_backlog);
	
	slistener->addrlen = rit->ai_addrlen;
	crtx_free_addrinfo(addrinfos);
	
	slistener->parent.shutdown = &shutdown_socket_raw_listener;
	
	slistener->parent.el_payload.fd = slistener->sockfd;
// 	slistener->parent.el_payload.event_flags = EPOLLIN;
	slistener->parent.el_payload.data = slistener;
	slistener->parent.el_payload.event_handler = &socket_raw_accept_handler;
	slistener->parent.el_payload.event_handler_name = "socket_raw_accept_handler";
	
	return &slistener->parent;
}


static char client_read_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *el_payload;
	struct crtx_socket_raw_listener *slist;
	
	
	el_payload = (struct crtx_event_loop_payload*) event->data.pointer;
	
	slist = (struct crtx_socket_raw_listener *) el_payload->data;
	
	slist->accept_cb(slist, el_payload->fd, 0);
	
	return 0;
}

struct crtx_listener_base *crtx_new_socket_raw_client_listener(void *options) {
	struct crtx_socket_raw_listener *slistener;
	struct addrinfo *addrinfos, *rit;
// 	int ret;
	
	slistener = (struct crtx_socket_raw_listener *) options;
	
	
	
	addrinfos = crtx_get_addrinfo(slistener);
	
	if (!addrinfos) {
		ERROR("cannot resolve %s %s\n", slistener->host, slistener->service);
		return 0;
	}
	
	for (rit = addrinfos; rit; rit = rit->ai_next) {
		slistener->sockfd = socket(rit->ai_family, rit->ai_socktype, rit->ai_protocol);
		
		if (slistener->sockfd < 0)
			continue;
		
		if (connect(slistener->sockfd, rit->ai_addr, rit->ai_addrlen) == 0)
			break;
		
		close(slistener->sockfd);
	}
	if (rit == 0) {
		ERROR("cannot connect to %s:%s: %s\n", slistener->host, slistener->service, strerror(errno));
		return 0;
	}
	
	crtx_free_addrinfo(addrinfos);
	
	slistener->parent.shutdown = &shutdown_socket_raw_listener;
	
	slistener->parent.el_payload.fd = slistener->sockfd;
	// 	slistener->parent.el_payload.event_flags = EPOLLIN;
	slistener->parent.el_payload.data = slistener;
	slistener->parent.el_payload.event_handler = &client_read_handler;
	slistener->parent.el_payload.event_handler_name = "client_read_handler";
	
	return &slistener->parent;
}

void crtx_socket_raw_init() {
}

void crtx_socket_raw_finish() {
}
