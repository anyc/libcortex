
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

struct event_task *task, *rp_cache;
struct event_graph *rl_graph;

static void handle(struct event *event, void *userdata) {
	struct ev_nf_queue_packet_msg *ev;
	struct event *rl_event;
	char *s;
	size_t len;
// 	struct nfq_thread_data *td;
	
	if (event->response)
		return;
	
// 	td = (struct nfq_thread_data *) userdata;
	
	printf("rec size %zu\n", event->data_size);
// 	if (event->data_size < sizeof(struct ev_nf_queue_packet_msg))
	if (!nfq_packet_msg_okay(event))
		return;
	
	ev = (struct ev_nf_queue_packet_msg*) event->data;
	
	// 	len = ev->hw_addr?strlen(ev->hw_addr):0+3+1;
	// 	s = (char*) malloc(len);
	// 	sprintf(s, "hw %s\n", ev->hw_addr);
	
	
	
	
	// 	struct sockaddr_in source,dest;
	char *ss, *sd, *proto, *msg;
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
	
	proto = nfq_proto2str(iph->protocol);
	msg = "(yes/no)";
	len = strlen(proto) + 1 + strlen(ss) + 1 + strlen(sd) + 1 + strlen(msg) + 1;
	s = (char*) malloc(len);
	sprintf(s, "%s %s %s %s", proto, ss, sd, msg);
	
	do {
		rl_event = new_event();
		rl_event->type = "readline/request";
		rl_event->data = s;
		rl_event->data_size = len;
		
		if (!rl_graph)
			rl_graph = get_graph_for_event_type("readline/request");
		
		add_event(rl_graph, rl_event);
		
		wait_on_event(rl_event);
		
		printf("response: %s\n", (char*) rl_event->response);
		
		if (rl_event->response && !strncmp(rl_event->response, "yes", 3)) {
			event->response = (void*) 1;
		}
	} while (!event->response && rl_event->response && strncmp(rl_event->response, "no", 2));
}

void init() {
	struct nfq_thread_data *td;
	
	td = (struct nfq_thread_data*) create_listener("nf_queue", 0);
	
	rp_cache = create_response_cache_task(&nfq_decision_cache_create_key);
	add_task(td->graph, rp_cache);
	
	task = new_task();
	task->id = "pfw_handler";
	task->handle = &handle;
	task->userdata = td;
	add_task(td->graph, task);
	
	print_tasks(td->graph);
}

void finish() {
	
}