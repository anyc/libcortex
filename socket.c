
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "core.h"
#include "socket.h"

static char *event_types[] = { "cortexd/enable", 0 };

void handle_enable(struct event *event) {
	
}

struct socket_thread_args {
	struct event_graph *graph;
	char stop;
};

void *socket_tmain(void *data) {
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct socket_req req;
	struct sockaddr_in serv_addr, cli_addr;
	int n, ret;
	struct socket_thread_args *sargs;
	struct event *event;
	
	sargs = (struct socket_thread_args*) data;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0); ASSERT(sockfd >= 0);
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	
	
	portno = 1234;
	serv_addr.sin_family = AF_INET;
// 	serv_addr.sin_addr.s_addr = INADDR_ANY;
	inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port = htons(portno);
	
	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)); ASSERT(ret >= 0);
	
	listen(sockfd, 5);
	
	while (!sargs->stop) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); ASSERT(newsockfd >= 0);
		
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
		
		event = (struct event*) calloc(1, sizeof(struct event));
		event->type = req.type;
		event->data = req.data;
		
		add_event(sargs->graph, event);
		
	// 	printf("Here is the message: %s\n",buffer);
	// 	n = write(newsockfd,"I got your message",18);
	// 	if (n < 0) error("ERROR writing to socket");
		
		close(newsockfd);
	}
	close(sockfd);
}

void socket_init() {
	unsigned int i;
	
	struct event_graph *event_graph;
	
	new_eventgraph(&event_graph, event_types);
	
	struct event_task *etask = (struct event_task*) malloc(sizeof(struct event_task));
	etask->handle = &handle_enable;
	event_graph->entry_task = etask;
	
	pthread_t thread;
	struct socket_thread_args sargs;
	
	sargs.graph = event_graph;
	sargs.stop = 0;
	
	pthread_create(&thread, NULL, socket_tmain, &sargs);
	
	pthread_join(thread, NULL);
}

