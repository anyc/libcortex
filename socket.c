
/*
 * To open a TCPv4 connection with localhost:1234
 * 
 * sock_listener.ai_family = AF_INET;
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

#include "core.h"
#include "socket.h"
#include "event_comm.h"
#include "threads.h"

struct socket_connection_tmain_args {
	struct crtx_socket_listener *slistener;
	int sockfd;
	struct crtx_thread_job_description thread_job;
	struct crtx_thread *thread;
};

static struct addrinfo *crtx_get_addrinfo(struct crtx_socket_listener *listener) {
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

static void crtx_free_addrinfo(struct addrinfo *result) {
	if (result->ai_family == AF_UNIX) {
		free(result);
	} else {
		freeaddrinfo(result);
	}
}

/// before original event is released, setup and write the response event
static void setup_response_event_cb(struct crtx_event *event, void *userdata) {
	struct crtx_socket_listener *slist;
	struct crtx_event *resp_event;
	
	slist = (struct crtx_socket_listener*) event->cb_before_release_data;
	
	// copy the data from the original event
	resp_event = crtx_create_event("cortex.socket.response");
	crtx_event_set_dict_data(resp_event, event->response.dict, 0);
	crtx_event_set_raw_data(resp_event, 'p', event->response.pointer, event->response.size, 0);
// 	resp_event->data.dict = event->response.dict;
	resp_event->original_event_id = event->original_event_id;
	
	// strip original event to prevent release of data
	event->response.pointer = 0;
// 	event->response.dict = 0;
	
	crtx_add_event(slist->outbox, resp_event);
}

static char outbound_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_socket_listener *slistener;
	
	slistener = (struct crtx_socket_listener *) userdata;
	
	if (!slistener->crtx_outbox)
		slistener->crtx_outbox = get_graph_for_event_type(CRTX_EVT_OUTBOX, crtx_evt_outbox);
	
	// write every outbound event through the main outbox graph
	crtx_traverse_graph(slistener->crtx_outbox, event);
	
	write_event_as_dict(event, crtx_wrapper_write, &slistener->sockfd);
	
	return 1;
}

static char inbound_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_socket_listener *slistener;
	
	slistener = (struct crtx_socket_listener *) userdata;
	
	if (!slistener->crtx_inbox)
		slistener->crtx_inbox = get_graph_for_event_type(CRTX_EVT_INBOX, crtx_evt_inbox);
	
	// write inbound event to the main inbox graph
	crtx_add_event(slistener->crtx_inbox, event);
	
	return 1;
}

/// thread for a connection of a server socket
static void *socket_connection_tmain(void *data) {
	struct crtx_socket_listener slist;
	struct crtx_event *event;
	struct socket_connection_tmain_args *args;
	
	
	args = (struct socket_connection_tmain_args*) data;
	
	memcpy(&slist, args->slistener, sizeof(struct crtx_socket_listener));
	
	crtx_create_graph(&slist.parent.graph, 0, slist.recv_types);
	crtx_create_graph(&slist.outbox, 0, slist.send_types);
	
	slist.sockfd = args->sockfd;
	
	crtx_create_task(slist.parent.graph, 200, "inbound_event_handler", &inbound_event_handler, &slist);
	
	crtx_create_task(slist.outbox, 200, "outbound_event_handler", &outbound_event_handler, &slist);
	
	while (1) {
		event = read_event_as_dict(&crtx_wrapper_read, &args->sockfd);
		if (!event)
			break;
		
		if (event->response_expected) {
			event->cb_before_release = &setup_response_event_cb;
			event->cb_before_release_data = &slist;
		}
		
		if (slist.parent.graph)
			crtx_add_event(slist.parent.graph, event);
		else
			add_raw_event(event);
	}
	
	// TODO handle aborted connection if events are still processed
	
	close(args->sockfd);
	
	return 0;
}

/// main thread of a server socket
static void *socket_server_tmain(void *data) {
	int ret;
	struct crtx_socket_listener *listeners;
	struct addrinfo *result, *rit;
	socklen_t addrlen;
	struct sockaddr *cliaddr;
	
	listeners = (struct crtx_socket_listener*) data;
	
	result = crtx_get_addrinfo(listeners);
	
	for (rit = result; rit; rit = rit->ai_next) {
		listeners->sockfd = socket(rit->ai_family, rit->ai_socktype, rit->ai_protocol);
		
		if (listeners->sockfd < 0)
			continue;
		
		int enable = 1;
		ret = setsockopt(listeners->sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
		if (ret < 0) {
			printf("setsockopt(SO_REUSEADDR) failed");
		}
		
		if (rit->ai_family == AF_UNIX) {
			ret = unlink( ((struct sockaddr_un *)rit->ai_addr)->sun_path );
			if (ret < 0)
				printf("failed to unlink existing unix socket: %s\n", strerror(errno));
		}
		
		if (bind(listeners->sockfd, rit->ai_addr, rit->ai_addrlen) == 0)
			break;
		
		close(listeners->sockfd);
	}
	
	if (rit == 0) {
		printf("error in socket main thread: %s\n", strerror(errno));
		return 0;
	}
	
	listen(listeners->sockfd, 5);
	
	addrlen=rit->ai_addrlen;
	
	crtx_free_addrinfo(result);
	
	cliaddr = malloc(addrlen);
	
	while (!listeners->stop) {
		struct socket_connection_tmain_args *args;
		
		args = (struct socket_connection_tmain_args *) malloc(sizeof(struct socket_connection_tmain_args));
		
		args->slistener = listeners;
		args->sockfd = accept(listeners->sockfd, cliaddr, &addrlen); ASSERT(args->sockfd >= 0);
		
// 		get_thread(socket_connection_tmain, args, 1);
		args->thread_job.fct = &socket_connection_tmain;
		args->thread_job.fct_data = args;
		
		args->thread = crtx_thread_assign_job(&args->thread_job);
		crtx_thread_start_job(args->thread);
	}
	close(listeners->sockfd);
	
	return 0;
}

/// main thread of a client socket
static void *socket_client_tmain(void *data) {
	struct crtx_socket_listener *listeners;
	struct crtx_event *event;
	struct addrinfo *result, *rit;
	
	listeners = (struct crtx_socket_listener*) data;
	
	result = crtx_get_addrinfo(listeners);
	
	for (rit = result; rit; rit = rit->ai_next) {
		listeners->sockfd = socket(rit->ai_family, rit->ai_socktype, rit->ai_protocol);
		
		if (listeners->sockfd < 0)
			continue;
		
		if (connect(listeners->sockfd, rit->ai_addr, rit->ai_addrlen) == 0)
			break;
		
		close(listeners->sockfd);
	}
	
	if (rit == 0) {
		printf("error in socket client main thread: %s\n", strerror(errno));
		return 0;
	}
	
	crtx_free_addrinfo(result);
	
	while (!listeners->stop) {
		event = read_event_as_dict(&crtx_wrapper_read, &listeners->sockfd);
		if (!event) {
			close(listeners->sockfd);
			break;
		}
		
		if (event->response_expected) {
			event->cb_before_release = &setup_response_event_cb;
			event->cb_before_release_data = listeners;
		}
		
		if (listeners->parent.graph)
			crtx_add_event(listeners->parent.graph, event);
		else
			add_raw_event(event);
	}
	close(listeners->sockfd);
	
	return 0;
}

static void stop_thread(struct crtx_thread *t, void *data) {
	struct crtx_socket_listener *inlist;
	
	DBG("stopping socket listener\n");
	
	inlist = (struct crtx_socket_listener*) data;
	
	inlist->stop = 1;
	if (inlist->sockfd)
		shutdown(inlist->sockfd, SHUT_RDWR);
	if (inlist->server_sockfd)
		shutdown(inlist->server_sockfd, SHUT_RDWR);
}

struct crtx_listener_base *crtx_new_socket_server_listener(void *options) {
	struct crtx_socket_listener *slistener;
// 	struct crtx_thread *t;
	
	slistener = (struct crtx_socket_listener *) options;
	
	create_in_out_box();
	
	slistener->parent.start_listener = 0;
// 	slistener->parent.thread = get_thread(socket_server_tmain, slistener, 0);
// 	slistener->parent.thread->do_stop = &stop_thread;
	slistener->parent.thread_job.fct = &socket_server_tmain;
	slistener->parent.thread_job.fct_data = slistener;
	slistener->parent.thread_job.do_stop = &stop_thread;
	
	return &slistener->parent;
}

struct crtx_listener_base *crtx_new_socket_client_listener(void *options) {
	struct crtx_socket_listener *slistener;
	
	slistener = (struct crtx_socket_listener *) options;
	
	create_in_out_box();
	
	crtx_create_graph(&slistener->parent.graph, 0, slistener->recv_types);
	crtx_create_graph(&slistener->outbox, 0, slistener->send_types);
	
	crtx_create_task(slistener->parent.graph, 200, "inbound_event_handler", &inbound_event_handler, slistener);
	crtx_create_task(slistener->outbox, 200, "outbound_event_handler", &outbound_event_handler, slistener);
	
	slistener->parent.start_listener = 0;
// 	slistener->parent.thread = get_thread(socket_client_tmain, slistener, 0);
	slistener->parent.thread_job.fct = &socket_client_tmain;
	slistener->parent.thread_job.fct_data = slistener;
	slistener->parent.thread_job.do_stop = &stop_thread;
	
	return &slistener->parent;
}

void crtx_socket_init() {
}

void crtx_socket_finish() {
}

