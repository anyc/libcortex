
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>


#include "core.h"
#include "nf_queue.h"
#include "plugins.h"

struct main_cb *cbs;
struct event_task task;
struct event_graph *rl_graph;

static void handle(struct event *event, void *userdata) {
	struct ev_nf_queue_packet_msg *ev;
	struct event *rl_event;
	char *s;
	size_t len;
	struct nfq_thread_data *td;
	
	
	td = (struct nfq_thread_data *) userdata;
	
	printf("rec size %zu\n", event->data_size);
	if (event->data_size < sizeof(struct ev_nf_queue_packet_msg))
		return;
	
	ev = (struct ev_nf_queue_packet_msg*) event->data;
	
	// 	len = ev->hw_addr?strlen(ev->hw_addr):0+3+1;
	// 	s = (char*) malloc(len);
	// 	sprintf(s, "hw %s\n", ev->hw_addr);
	
	
	
	
	// 	struct sockaddr_in source,dest;
	char *ss, *sd;
	// 	unsigned short iphdrlen;
	struct iphdr *iph = (struct iphdr*)ev->payload;
	
	// 	iphdrlen =iph->ihl*4;
	
	// 	memset(&source, 0, sizeof(source));
	// 	source.sin_addr.s_addr = iph->saddr;
	// 	
	// 	memset(&dest, 0, sizeof(dest));
	// 	dest.sin_addr.s_addr = iph->daddr;
	// 	
	// 	ss = inet_ntoa(source.sin_addr);
	// 	sd = inet_ntoa(dest.sin_addr);
	
	ss = inet_ntoa(*(struct in_addr *)&iph->saddr);
	sd = inet_ntoa(*(struct in_addr *)&iph->daddr);
	
	len = 2 + strlen(ss) + 1 + strlen(sd) + 1;
	s = (char*) malloc(len);
	sprintf(s, "%d %s %s\n", iph->protocol, ss, sd);
	
	
	rl_event = cbs->new_event();
	rl_event->type = "readline/request";
	rl_event->data = s;
	rl_event->data_size = len;
	
	if (!rl_graph)
		rl_graph = cbs->get_graph_for_event_type("readline/request");
	
	cbs->add_event(rl_graph, rl_event);
	
	cbs->wait_on_event(rl_event);
	
	printf("response: %s\n", (char*) rl_event->response);
	
	if (rl_event->response && !strncmp(rl_event->response, "yes", 3)) {
		event->response = (void*) 1;
	}
}

void init(struct main_cb *main_cbs) {
	struct nfq_thread_data *td;
	
	printf("pfw init\n");
	
	cbs = main_cbs;
	
	td = (struct nfq_thread_data*) cbs->create_listener("nf_queue", 0);
	
	task.handle = &handle;
	task.userdata = td;
	cbs->add_task(td->graph, &task, 255);
}

void finish() {
	
}