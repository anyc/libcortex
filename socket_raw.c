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
#include <fcntl.h>

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

static int create_sub_lstnr(struct crtx_socket_raw_listener **listener, struct crtx_socket_raw_listener *server_lstnr, int fd) {
	struct crtx_socket_raw_listener *lstnr;
	
	*listener = (struct crtx_socket_raw_listener*) calloc(1, sizeof(struct crtx_socket_raw_listener));
	lstnr = *listener;
	
// 	lstnr->read_cb = slist->read_cb;
// 	lstnr->read_cb_data = slist->read_cb_data;
// 	lstnr->fd = fd;
// 	lstnr->sock_lstnr = slist;
	
	memcpy(lstnr, server_lstnr, sizeof(struct crtx_socket_raw_listener));
	lstnr->server_lstnr = server_lstnr;
	lstnr->sockfd = fd;
	lstnr->base.id = "basic_fd";
	
	crtx_init_listener_base(&lstnr->base);
	
	crtx_evloop_init_listener(&lstnr->base,
							&lstnr->sockfd, 0,
							CRTX_EVLOOP_READ,
							0,
							lstnr->read_cb,
							lstnr,
							0, 0
						);
	
	return 0;
}

static char socket_raw_accept_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_socket_raw_listener *slist;
	struct sockaddr *cliaddr;
	int fd, r;
// 	struct crtx_evloop_callback *el_cb;
	
	
// 	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	slist = (struct crtx_socket_raw_listener *) userdata;
	
	cliaddr = malloc(slist->addrlen);
	fd = accept(slist->sockfd, cliaddr, &slist->addrlen);
	if (slist->sockfd < 0) {
		CRTX_ERROR("accept failed: %d\n", fd);
		return 0;
	}
	
	r = slist->accept_cb(slist, fd, cliaddr);
	if (r < 0) {
		
	} else {
		struct crtx_socket_raw_listener *sub_lstnr;
		
		r = create_sub_lstnr(&sub_lstnr, slist, fd);
		if (r < 0) {
			
		}
		
		crtx_start_listener(&sub_lstnr->base);
	}
	
	return 0;
}

// static char socket_raw_accept_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct crtx_event *event;
// 	
// 	event = crtx_create_event(0);
// 	
// 	crtx_event_set_raw_data(nevent, 'p', evt, sizeof(eXosip_event_t*), CRTX_DIF_DONT_FREE_DATA);
// 	
// 	nevent->cb_before_release = &sip_event_before_release_cb;
// 	
// 	crtx_add_event(slist->base.graph, event);
// 	
// 	return 0;
// }

void shutdown_socket_raw_listener(struct crtx_listener_base *data) {
	struct crtx_socket_raw_listener *slist;
	
	slist = (struct crtx_socket_raw_listener*) data;
	
	if (slist->sockfd >= 0)
		close(slist->sockfd);
}

// struct crtx_listener_base *crtx_setup_socket_raw_server_listener(void *options) {
int crtx_init_socket_raw_server_listener(struct crtx_socket_raw_listener *slistener) {
	struct addrinfo *addrinfos, *rit;
	int ret;
	
// 	slistener = (struct crtx_socket_raw_listener *) options;
	
	
	slistener->base.id = "socket_raw_server";
	
	crtx_init_listener_base(&slistener->base);
	
	addrinfos = crtx_get_addrinfo(slistener);
	
	for (rit = addrinfos; rit; rit = rit->ai_next) {
		slistener->sockfd = socket(rit->ai_family, rit->ai_socktype, rit->ai_protocol);
		
		if (slistener->sockfd < 0)
			continue;
		
		int enable = 1;
		ret = setsockopt(slistener->sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
		if (ret < 0) {
			CRTX_ERROR("setsockopt(SO_REUSEADDR) failed");
		}
		
		if (rit->ai_family == AF_UNIX) {
			ret = unlink( ((struct sockaddr_un *)rit->ai_addr)->sun_path );
			if (ret < 0)
				CRTX_ERROR("failed to unlink existing unix socket: %s\n", strerror(errno));
		}
		
		int flags;
		flags = fcntl(slistener->sockfd, F_GETFL, 0);
		if (flags == -1)
			flags = 0;
		fcntl(slistener->sockfd, F_SETFL, flags | O_NONBLOCK);
		
		if (bind(slistener->sockfd, rit->ai_addr, rit->ai_addrlen) == 0)
			break;
		
		close(slistener->sockfd);
	}
	if (rit == 0) {
		CRTX_ERROR("crtx_setup_socket_raw_server_listener: %s\n", strerror(errno));
		return errno;
	}
	
	slistener->addrlen = rit->ai_addrlen;
	crtx_free_addrinfo(addrinfos);
	
	slistener->base.shutdown = &shutdown_socket_raw_listener;
	
	if (slistener->type == SOCK_STREAM) {
		if (slistener->listen_backlog == 0)
			slistener->listen_backlog = 5;
		
		listen(slistener->sockfd, slistener->listen_backlog);
		
		crtx_evloop_init_listener(&slistener->base,
						slistener->sockfd,
						CRTX_EVLOOP_READ,
						0,
						&socket_raw_accept_handler,
						slistener,
						0, 0
					);
	} else 
	if (slistener->type == SOCK_DGRAM) {
		crtx_evloop_init_listener(&slistener->base,
						slistener->sockfd,
						CRTX_EVLOOP_READ,
						0,
						slistener->read_cb,
						slistener,
						0, 0
					);
	} else {
		CRTX_ERROR("invalid socket type %d\n", slistener->type);
		return -EINVAL;
	}
	
// 	return &slistener->base;
	return 0;
}


static char client_read_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct crtx_evloop_fd *evloop_fd;
	struct crtx_socket_raw_listener *slist;
	
// 	struct crtx_evloop_callback *el_cb;
	
// 	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
// 	evloop_fd = (struct crtx_evloop_fd*) event->data.pointer;
	
	slist = (struct crtx_socket_raw_listener *) userdata;
	
	slist->accept_cb(slist, slist->sockfd, 0);
	
	return 0;
}

int crtx_init_socket_raw_client_listener(struct crtx_socket_raw_listener *slistener) {
	struct addrinfo *addrinfos, *rit;
	
	
	slistener->base.id = "socket_raw_client";
	
	crtx_init_listener_base(&slistener->base);
	
	addrinfos = crtx_get_addrinfo(slistener);
	
	if (!addrinfos) {
		CRTX_ERROR("cannot resolve %s %s\n", slistener->host, slistener->service);
		return -ENXIO;
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
		CRTX_ERROR("cannot connect to %s:%s: %s\n", slistener->host, slistener->service, strerror(errno));
		return -EHOSTUNREACH;
	}
	
	crtx_free_addrinfo(addrinfos);
	
	slistener->base.shutdown = &shutdown_socket_raw_listener;
	
// 	slistener->base.evloop_fd.fd = slistener->sockfd;
// 	// 	slistener->base.evloop_fd.crtx_event_flags = CRTX_EVLOOP_READ;
// 	slistener->base.evloop_fd.data = slistener;
// 	slistener->base.evloop_fd.event_handler = &client_read_handler;
// 	slistener->base.evloop_fd.event_handler_name = "client_read_handler";
	crtx_evloop_init_listener(&slistener->base,
						slistener->sockfd,
						CRTX_EVLOOP_READ,
						0,
						&client_read_handler,
						slistener,
						0, 0
					);
	
	return 0;
}

// struct crtx_listener_base *crtx_setup_socket_raw_client_listener(void *options) {
// 	struct crtx_socket_raw_listener *slistener;
// 	struct addrinfo *addrinfos, *rit;
// 	int r;
// 	
// 	slistener = (struct crtx_socket_raw_listener *) options;
// 	
// 	r = crtx_init_socket_raw_client_listener(slistener);
// 	if (r < 0) [
// 		return 0;
// 	}
// 	
// 	return &slistener->base;
// }

struct crtx_listener_base *crtx_setup_socket_raw_listener(void *options) {
	return 0;
}

CRTX_DEFINE_ALLOC_FUNCTION(socket_raw)

void crtx_socket_raw_init() {
}

void crtx_socket_raw_finish() {
}
