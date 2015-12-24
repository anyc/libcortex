
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


#include "core.h"
#include "socket.h"

static char *event_types[] = { "cortexd/enable", 0 };

void *socket_tmain(void *data) {
	int sockfd, newsockfd;
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
		sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		
		if (sockfd < 0)
			continue;
		
// 		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		
		if (bind(sockfd, result->ai_addr, result->ai_addrlen)==0)
			break;
		
		close(sockfd);
	} while ((result=result->ai_next) != NULL);
	
	if (result == 0) {
		printf("error in socket main thread\n");
		return 0;
	}
	
	listen(sockfd, 5);
	
	addrlen=result->ai_addrlen;
	
	freeaddrinfo(resbkp);
	
	cliaddr = malloc(addrlen);
	
	while (!listeners->stop) {
// 		clilen = sizeof(cli_addr);
// 		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); ASSERT(newsockfd >= 0);
		newsockfd = accept(sockfd, cliaddr, &addrlen); ASSERT(newsockfd >= 0);
		
		n = read(newsockfd, &req, sizeof(struct socket_req));
		if (n != sizeof(struct socket_req)) {
			printf("invalid data received\n");
			close(newsockfd);
			continue;
		}
		
		req.type = (char*) calloc(1, req.type_length);
		
		n = read(newsockfd, req.type, req.type_length);
		if (n != req.type_length) {
			printf("invalid data received\n");
			free(req.type);
			close(newsockfd);
			continue;
		}
		
		req.data = (char*) calloc(1, req.data_length);
		
		n = read(newsockfd, req.data, req.data_length);
		if (n != req.data_length) {
			printf("invalid data received\n");
			free(req.type);
			free(req.data);
			close(newsockfd);
			continue;
		}
		
		event = new_event();
		event->type = req.type;
		event->data = req.data;
		
		add_event(listeners->graph, event);
		
	// 	printf("Here is the message: %s\n",buffer);
	// 	n = write(newsockfd,"I got your message",18);
	// 	if (n < 0) error("ERROR writing to socket");
		
		close(newsockfd);
	}
	close(sockfd);
	
	return 0;
}

void free_socket_listener(void *data) {
	struct socket_listener *slistener = (struct socket_listener *) slistener;
	
	free_eventgraph(slistener->parent.graph);
	
// 	pthread_join(slistener->thread, 0);
	
	free(slistener);
}

struct listener *new_socket_listener(void *options) {
// 	struct inotify_listener *inlist;
// 	
// 	inlist = (struct inotify_listener*) options;
// 	
// 	if (inotify_fd == -1) {
// 		inotify_fd = inotify_init();
// 		
// 		if (inotify_fd == -1) {
// 			printf("inotify initialization failed\n");
// 			return 0;
// 		}
// 	}
// 	
// 	inlist->wd = inotify_add_watch(inotify_fd, inlist->path, inlist->mask);
// 	if (inlist->wd == -1) {
// 		printf("inotify_add_watch(\"%s\") failed with %d: %s\n", (char*) options, errno, strerror(errno));
// 		return 0;
// 	}
// 	
// 	inlist->parent.free = &free_inotify_listener;
// 	
// 	new_eventgraph(&inlist->parent.graph, inotify_msg_etype);
// 	
// 	pthread_create(&inlist->thread, NULL, inotify_tmain, inlist);
	
	struct socket_listener *slistener;
	
	slistener = (struct socket_listener *) options;
	
	slistener->parent.free = &free_socket_listener;
	
	new_eventgraph(&slistener->parent.graph, event_types);
	
	pthread_create(&slistener->thread, NULL, socket_tmain, slistener);
	
// 	new_eventgraph(&event_graph, event_types);
	
// 	struct event_task *etask = new_task();
// 	etask->handle = &handle_enable;
// 	event_graph->tasks = etask;
	
// 	sargs.graph = event_graph;
// 	sargs.stop = 0;
	
// 	pthread_create(&thread, NULL, socket_tmain, &sargs);
	
	return &slistener->parent;
}

void socket_init() {
// 	struct event_graph *event_graph;
// 	
// 	new_eventgraph(&event_graph, event_types);
// 	
// 	struct event_task *etask = new_task();
// 	etask->handle = &handle_enable;
// 	event_graph->tasks = etask;
// 	
// 	sargs.graph = event_graph;
// 	sargs.stop = 0;
// 	
// 	pthread_create(&thread, NULL, socket_tmain, &sargs);
}

void socket_finish() {
// 	pthread_join(thread, NULL);
}

