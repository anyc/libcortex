
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
#include "event_comm.h"

struct socket_connection_tmain_args {
	pthread_t thread;
	
	struct socket_listener *slistener;
	int sockfd;
};

int socket_recv(void *conn_id, void *data, size_t data_size) {
	return read(*(int*) conn_id, data, data_size);
}

int socket_send(void *conn_id, void *data, size_t data_size) {
	return write(*(int*) conn_id, data, data_size);
}

void event_before_release(struct crtx_event *event) {
// void event_respond_cb(struct crtx_event *event, struct crtx_event_data *ed) {
	struct socket_listener *slist;
	struct crtx_event *resp_event;
	
	slist = (struct socket_listener*) event->cb_before_release_data;
	printf("before release\n");
	resp_event = create_event("cortex/socket/response", event->response.raw, event->response.raw_size);
	resp_event->data.dict = event->response.dict;
// 	resp_event->original_event = event;
// 	resp_event->original_event_id = (uint64_t) (uintptr_t) event;
	resp_event->original_event_id = event->original_event_id;
	
	event->response.raw = 0;
	event->response.dict = 0;
	
	add_event(slist->outbox, resp_event);
}

static void send_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct socket_listener *slistener;
	
	slistener = (struct socket_listener *) userdata;
	
	if (!slistener->crtx_outbox)
		slistener->crtx_outbox = get_graph_for_event_type(CRTX_EVT_OUTBOX, crtx_evt_outbox);
	
	crtx_traverse_graph(slistener->crtx_outbox, event);
	
	send_event_as_dict(event, socket_send, &slistener->sockfd);
}

static void recv_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct socket_listener *slistener;
	
	slistener = (struct socket_listener *) userdata;
	
	if (!slistener->crtx_inbox)
		slistener->crtx_inbox = get_graph_for_event_type(CRTX_EVT_INBOX, crtx_evt_inbox);
	
	add_event(slistener->crtx_inbox, event);
}

void *socket_connection_tmain(void *data) {
	struct socket_listener slist;
	struct crtx_event *event;
	struct socket_connection_tmain_args *args;
	
	
	args = (struct socket_connection_tmain_args*) data;
	
	memcpy(&slist, args->slistener, sizeof(struct socket_listener));
	
	new_eventgraph(&slist.parent.graph, slist.recv_types);
	new_eventgraph(&slist.outbox, slist.send_types);
	
	slist.sockfd = args->sockfd;
	
	struct crtx_task * recv_event;
	recv_event = new_task();
	recv_event->id = "recv_event";
	recv_event->handle = &recv_event_handler;
	recv_event->userdata = &slist;
	recv_event->position = 200;
	add_task(slist.parent.graph, recv_event);
	
	struct crtx_task * task;
	task = new_task();
	task->id = "send_event";
	task->handle = &send_event_handler;
	task->userdata = &slist;
	task->position = 200;
	add_task(slist.outbox, task);
	
	while (1) {
		event = recv_event_as_dict(&socket_recv, &args->sockfd);
		if (!event)
			break;
		
// 		event->respond_cb_payload = &slist;
// 		event->respond = &event_respond_cb;
// 		if (event->refs_before_release > 0) {
		if (event->response_expected) {
			event->cb_before_release = &event_before_release;
			event->cb_before_release_data = &slist;
		}
		
		
		if (slist.parent.graph)
			add_event(slist.parent.graph, event);
		else
			add_raw_event(event);
	}
	
	// TODO handle aborted connection if events are still processed
	
	close(args->sockfd);
	
	return 0;
}

void *socket_server_tmain(void *data) {
	int ret;
	struct socket_listener *listeners;
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
		struct socket_connection_tmain_args *args;
		
		args = (struct socket_connection_tmain_args *) malloc(sizeof(struct socket_connection_tmain_args));
		
		args->slistener = listeners;
		args->sockfd = accept(listeners->sockfd, cliaddr, &addrlen); ASSERT(args->sockfd >= 0);
		
		pthread_create(&args->thread, NULL, socket_connection_tmain, args);
	}
	close(listeners->sockfd);
	
	return 0;
}

void *socket_client_tmain(void *data) {
	int ret;
	struct socket_listener *listeners;
	struct crtx_event *event;
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
		event = recv_event_as_dict(&socket_recv, &listeners->sockfd);
		if (!event) {
			close(listeners->sockfd);
			break;
		}
		
// 		if (event->refs_before_release > 0) {
		if (event->response_expected) {
			event->cb_before_release = &event_before_release;
			event->cb_before_release_data = &listeners;
		}
		
		if (listeners->parent.graph)
			add_event(listeners->parent.graph, event);
		else
			add_raw_event(event);
	}
	close(listeners->sockfd);
	
	return 0;
}

struct crtx_listener_base *new_socket_server_listener(void *options) {
	struct socket_listener *slistener;
	
	slistener = (struct socket_listener *) options;
	
	create_in_out_box();
	
// 	new_eventgraph(&slistener->parent.graph, slistener->recv_types);
// 	new_eventgraph(&slistener->outbox, slistener->send_types);
// 	
// 	struct crtx_task * recv_event;
// 	recv_event = new_task();
// 	recv_event->id = "recv_event";
// 	recv_event->handle = &recv_event_handler;
// 	recv_event->userdata = slistener;
// 	recv_event->position = 200;
// 	add_task(slistener->parent.graph, recv_event);
// 	
// 	struct crtx_task * task;
// 	task = new_task();
// 	task->id = "send_event";
// 	task->handle = &send_event_handler;
// 	task->userdata = slistener;
// 	task->position = 200;
// 	add_task(slistener->outbox, task);
	
	pthread_create(&slistener->thread, NULL, socket_server_tmain, slistener);
	
	return &slistener->parent;
}

struct crtx_listener_base *new_socket_client_listener(void *options) {
	struct socket_listener *slistener;
	struct crtx_task * task;
	
	slistener = (struct socket_listener *) options;
	
	create_in_out_box();
	
	new_eventgraph(&slistener->parent.graph, slistener->recv_types);
	new_eventgraph(&slistener->outbox, slistener->send_types);
	
	struct crtx_task * recv_event;
	recv_event = new_task();
	recv_event->id = "recv_event";
	recv_event->handle = &recv_event_handler;
	recv_event->userdata = slistener;
	recv_event->position = 200;
	add_task(slistener->parent.graph, recv_event);
	
	task = new_task();
	task->id = "send_event";
	task->handle = &send_event_handler;
	task->userdata = slistener;
	task->position = 200;
	add_task(slistener->outbox, task);
	
	pthread_create(&slistener->thread, NULL, socket_client_tmain, slistener);
	
	return &slistener->parent;
}

void socket_init() {
}

void socket_finish() {
}

