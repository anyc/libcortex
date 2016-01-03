
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "core.h"
#include "socket.h"

void *socket_server_tmain(void *data) {
	int newsockfd;
// 	socklen_t clilen;
	struct socket_req req;
// 	struct sockaddr_in serv_addr, cli_addr;
	int n, ret;
	struct socket_listener *listeners;
	struct event *event;
	struct addrinfo hints, *result, *resbkp;
	socklen_t addrlen;
	struct sockaddr *cliaddr;
	
	listeners = (struct socket_listener*) data;
	
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_flags=AI_PASSIVE;
	hints.ai_family=listeners->ai_family;
	hints.ai_socktype=listeners->type;
	hints.ai_protocol=listeners->protocol;
	
	ret = getaddrinfo(listeners->host, listeners->service, &hints, &result);
	if (ret !=0) {
		printf("getaddrinfo error %s %s: %s", listeners->host, listeners->service, gai_strerror(ret));
		return 0;
	}
	
	resbkp = result;
	
// 	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol); ASSERT(sockfd >= 0);
// 	bzero((char *) &serv_addr, sizeof(serv_addr));
// 	serv_addr.sin_family = listeners->domain;
// // 	serv_addr.sin_addr.s_addr = INADDR_ANY;
// 	inet_pton(listeners->domain, "127.0.0.1", &serv_addr.sin_addr);
// 	serv_addr.sin_port = htons(listeners->port);
// 	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)); ASSERT(ret >= 0);
	
	do {
		listeners->sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		
		if (listeners->sockfd < 0)
			continue;
		
		int enable = 1;
		if (setsockopt(listeners->sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
			printf("setsockopt(SO_REUSEADDR) failed");
		}
		
		if (bind(listeners->sockfd, result->ai_addr, result->ai_addrlen) == 0)
			break;
		
		close(listeners->sockfd);
	} while ((result=result->ai_next) != NULL);
	
	if (result == 0) {
		printf("error in socket main thread\n");
		return 0;
	}
	
	listen(listeners->sockfd, 5);
	
	addrlen=result->ai_addrlen;
	
	freeaddrinfo(resbkp);
	
	cliaddr = malloc(addrlen);
	
	while (!listeners->stop) {
// 		clilen = sizeof(cli_addr);
// 		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); ASSERT(newsockfd >= 0);
		newsockfd = accept(listeners->sockfd, cliaddr, &addrlen); ASSERT(newsockfd >= 0);
		
		while (1) {
			n = read(newsockfd, &req, sizeof(struct socket_req));
			if (n == 0) {
				break;
			} else
			if (n < 0) {
				printf("error %d while reading from socket\n", n);
				break;
			} else
			if (n != sizeof(struct socket_req)) {
				printf("invalid request received\n");
				close(newsockfd);
				break;
			}
			
			req.type = (char*) calloc(1, req.type_length+1);
			
			n = read(newsockfd, req.type, req.type_length);
			if (n == 0) {
				break;
			} else
			if (n < 0) {
				printf("error %d while reading from socket\n", n);
				break;
			} else
			if (n != req.type_length) {
				printf("invalid req type received\n");
				free(req.type);
				close(newsockfd);
				break;
			}
			req.type[req.type_length] = 0;
			
			req.data = (char*) calloc(1, req.data_length);
			
			n = read(newsockfd, req.data, req.data_length);
			if (n == 0) {
				break;
			} else
			if (n < 0) {
				printf("error %d while reading from socket\n", n);
				break;
			} else
			if (n != req.data_length) {
				printf("invalid req data received\n");
				free(req.type);
				free(req.data);
				close(newsockfd);
				break;
			}
			
			event = new_event();
			event->type = req.type;
			event->data = req.data;
			event->response_graph = listeners->response_graph;
			
			if (listeners->parent.graph)
				add_event(listeners->parent.graph, event);
			else
				add_raw_event(event);
		}
		// 	printf("Here is the message: %s\n",buffer);
		// 	n = write(newsockfd,"I got your message",18);
		// 	if (n < 0) error("ERROR writing to socket");
			
// 			close(newsockfd);
	}
	close(listeners->sockfd);
	
	return 0;
}

void *socket_client_tmain(void *data) {
	struct socket_req req;
	int n, ret;
	struct socket_listener *listeners;
	struct event *event;
	struct addrinfo hints, *result, *resbkp;
	
	listeners = (struct socket_listener*) data;
	
	bzero(&hints, sizeof(struct addrinfo));
// 	hints.ai_flags=AI_PASSIVE;
	hints.ai_family=listeners->ai_family;
	hints.ai_socktype=listeners->type;
	hints.ai_protocol=listeners->protocol;
	
	ret = getaddrinfo(listeners->host, listeners->service, &hints, &result);
	if (ret !=0) {
		printf("getaddrinfo error %s %s: %s", listeners->host, listeners->service, gai_strerror(ret));
		return 0;
	}
	
	resbkp = result;
	
	do {
		listeners->sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		
		if (listeners->sockfd < 0)
			continue;
		
		if (connect(listeners->sockfd, result->ai_addr, result->ai_addrlen) == 0) {
// 			doit(sockfd,iobuf);
// 			printf("connection ok!\n"); /* success*/
			break;
		} else {
// 			printf("connection failed");
		}
		
		close(listeners->sockfd);
	} while ((result=result->ai_next) != NULL);
	
	if (result == 0) {
		printf("error in socket client main thread\n");
		return 0;
	}
	
	freeaddrinfo(resbkp);
	
	while (!listeners->stop) {
		n = read(listeners->sockfd, &req, sizeof(struct socket_req));
		if (n == 0) {
			break;
		} else
		if (n < 0) {
			printf("error %d while reading from socket\n", n);
			break;
		} else
		if (n != sizeof(struct socket_req)) {
			printf("invalid request received\n");
			close(listeners->sockfd);
			break;
		}
		
		req.type = (char*) calloc(1, req.type_length+1);
		
		n = read(listeners->sockfd, req.type, req.type_length);
		if (n == 0) {
			break;
		} else
		if (n < 0) {
			printf("error %d while reading from socket\n", n);
			break;
		} else
		if (n != req.type_length) {
			printf("invalid req type received\n");
			free(req.type);
			close(listeners->sockfd);
			break;
		}
		req.type[req.type_length] = 0;
		
		req.data = (char*) calloc(1, req.data_length);
		
		n = read(listeners->sockfd, req.data, req.data_length);
		if (n == 0) {
			break;
		} else
		if (n < 0) {
			printf("error %d while reading from socket\n", n);
			break;
		} else
		if (n != req.data_length) {
			printf("invalid req data received\n");
			free(req.type);
			free(req.data);
			close(listeners->sockfd);
			break;
		}
		
		event = new_event();
		event->type = req.type;
		event->data = req.data;
		event->response_graph = listeners->response_graph;
		
		if (listeners->parent.graph)
			add_event(listeners->parent.graph, event);
		else
			add_raw_event(event);
	}
	close(listeners->sockfd);
	
	return 0;
}

void send_event(struct socket_listener *slistener, struct event *event) {
	struct socket_req req;
	int ret;
	
	req.type = event->type;
	req.type_length = strlen(event->type);
	
	req.data = event->data;
	req.data_length = event->data_size;
	
	ret = write(slistener->sockfd, &req, sizeof(struct socket_req));
	if (ret < 0) {
		printf("sending event failed: %s\n", strerror(errno));
		return;
	}
	
	ret = write(slistener->sockfd, req.type, req.type_length);
	if (ret < 0) {
		printf("sending event failed: %s\n", strerror(errno));
		return;
	}
	
	ret = write(slistener->sockfd, req.data, req.data_length);
	if (ret < 0) {
		printf("sending event failed: %s\n", strerror(errno));
		return;
	}
}

static void return_event_handler(struct event *event, void *userdata, void **sessiondata) {
	struct socket_listener *slistener;
	
	slistener = (struct socket_listener *) userdata;
	
	send_event(slistener, event);
}

struct listener *new_socket_server_listener(void *options) {
	struct socket_listener *slistener;
	
	slistener = (struct socket_listener *) options;
	
// 	slistener->parent.graph = 0;
	
	new_eventgraph(&slistener->response_graph, 0);
	
	struct event_task * return_event;
	return_event = new_task();
	return_event->id = "return_event";
	return_event->handle = &return_event_handler;
	return_event->userdata = slistener;
	add_task(slistener->response_graph, return_event);
	
	pthread_create(&slistener->thread, NULL, socket_server_tmain, slistener);
	
	return &slistener->parent;
}

struct listener *new_socket_client_listener(void *options) {
	struct socket_listener *slistener;
	
	slistener = (struct socket_listener *) options;
	
// 	slistener->parent.graph = 0;
	
	new_eventgraph(&slistener->response_graph, 0);
	
	struct event_task * return_event;
	return_event = new_task();
	return_event->id = "return_event";
	return_event->handle = &return_event_handler;
	return_event->userdata = slistener;
	add_task(slistener->response_graph, return_event);
	
	pthread_create(&slistener->thread, NULL, socket_client_tmain, slistener);
	
	return &slistener->parent;
}

void socket_init() {
}

void socket_finish() {
}

