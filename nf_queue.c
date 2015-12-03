
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/netfilter.h>

#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "core.h"
#include "nf_queue.h"

// iptables -A INPUT -p tcp --dport 631 -j NFQUEUE --queue-num 0

struct nfq_thread_data {
	struct event_graph *graph;
};

struct ev_nf_queue_packet {
	struct nfq_q_handle *qh;
	struct nfq_data *data;
};

struct ev_nf_queue_packet_msg {
	int id;
	u_int16_t hw_protocol;
	
// 	u_int8_t hw_addr;
// 	u_int16_t hw_addrlen;
	char *hw_addr;
	
	u_int32_t mark;
	u_int32_t indev;
	u_int32_t outdev;
	u_int32_t physindev;
	u_int32_t physoutdev;
	
	unsigned char *payload;
	size_t payload_size;
};

static pthread_t pthread;
struct nfq_thread_data tdata;

#define NF_QUEUE_NEW_PACKET_TYPE "nf_queue/packet"
struct event_graph *new_packet_graph = 0;

#define NF_QUEUE_NEW_PACKET_MSG_TYPE "nf_queue/packet_msg"
struct event_graph *new_packet_graph_msg = 0;



static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
		    struct nfq_data *tb, void *data)
{
// 	int verdict;
// 	u_int32_t id = treat_pkt(nfa, &verdict); /* Treat packet */
// 	return nfq_set_verdict(qh, id, verdict, 0, NULL); /* Verdict packet */
	
	
// 	int id;
	struct nfqnl_msg_packet_hdr *ph;
	struct nfqnl_msg_packet_hw *hwph;
	u_int32_t mark,ifi; 
// 	int ret;
// 	char *data;
	
// 	id = 0;
// 	ph = nfq_get_msg_packet_hdr(tb);
// 	if (ph) {
// 		id = ntohl(ph->packet_id);
// 		printf("hw_protocol=0x%04x hook=%u id=%u ",
// 			  ntohs(ph->hw_protocol), ph->hook, id);
// 	}
// 	
// 	hwph = nfq_get_packet_hw(tb);
// 	if (hwph) {
// 		int i, hlen = ntohs(hwph->hw_addrlen);
// 		
// 		printf("hw_src_addr=");
// 		for (i = 0; i < hlen-1; i++)
// 			printf("%02x:", hwph->hw_addr[i]);
// 		printf("%02x ", hwph->hw_addr[hlen-1]);
// 	}
// 	
// 	mark = nfq_get_nfmark(tb);
// 	if (mark)
// 		printf("mark=%u ", mark);
// 	
// 	ifi = nfq_get_indev(tb);
// 	if (ifi)
// 		printf("indev=%u ", ifi);
// 	
// 	ifi = nfq_get_outdev(tb);
// 	if (ifi)
// 		printf("outdev=%u ", ifi);
// 	
// 	ifi = nfq_get_physindev(tb);
// 	if (ifi)
// 		printf("physindev=%u ", ifi);
// 	
// 	ifi = nfq_get_physoutdev(tb);
// 	if (ifi)
// 		printf("physoutdev=%u ", ifi);
	
// 	ret = nfq_get_payload(tb, &data);
// 	if (ret >= 0) {
// 		printf("payload_len=%d ", ret);
// 		processPacketData (data, ret);
// 	}
// 	fputc('\n', stdout);
	printf("asd\n");
	
	
// 	unsigned char *payload;
// 	int payload_length;
// 	
// 	payload_length = nfq_get_payload(tb, &payload);
	
// 	struct iphdr * ip_info = (struct iphdr *)payload;
// 	
// 	if(ipinfo->protocol == IPPROTO_TCP) {
// 		struct tcphdr * tcp_info = (struct * tcphdr)(payload + sizeof(*ip_info));
// 		unsigned short dest_port = ntohs(tcp_info->dest);
// 	} else if(ipinfo->protocol == IPPROTO_UDP) {
// 		// etc etc
// 	}
	
// 	event->data = malloc(payload_length);
// 	memcpy(event->data, payload, payload_length);
	
	
	
	
// 	if (!new_packet_graph)
// 		new_packet_graph = get_graph_for_event_type(NF_QUEUE_NEW_PACKET_TYPE);
// 	
// 	if (new_packet_graph && new_packet_graph->tasks) {
// 		struct event *event;
// 		
// 		event = (struct event*) malloc(sizeof(struct event));
// 		event->type = NF_QUEUE_NEW_PACKET_TYPE;
// 		
// 		struct ev_nf_queue_packet *ev;
// 		ev = malloc(sizeof(struct ev_nf_queue_packet));
// 		event->data = ev;
// 		
// 		ev->qh = qh;
// 		ev->data = tb;
// 		
// 		add_event(new_packet_graph, event);
// 	}
	
	
	if (!new_packet_graph_msg)
		new_packet_graph_msg = get_graph_for_event_type(NF_QUEUE_NEW_PACKET_MSG_TYPE);
	
	if (new_packet_graph_msg && new_packet_graph_msg->tasks) {
		struct ev_nf_queue_packet_msg *ev;
		struct event *event;
		size_t msg_size;
		unsigned char *payload;
		size_t payload_size, data_size;
		u_int16_t hw_addrlen;
		
		
		msg_size = 0;
		
		hwph = nfq_get_packet_hw(tb);
		if (hwph) {
			hw_addrlen = ntohs(hwph->hw_addrlen);
			msg_size += hw_addrlen*3 + 1;
		} else
			hw_addrlen = 0;
		
		payload_size = nfq_get_payload(tb, &payload);
		msg_size += payload_size;
		
// 		printf("%lu %zu %lu\n", msg_size + sizeof(struct ev_nf_queue_packet_msg), msg_size, sizeof(struct ev_nf_queue_packet_msg));
		data_size = msg_size + sizeof(struct ev_nf_queue_packet_msg);
		ev = calloc(1, data_size);
		
		if (hwph) {
			int i;
			
			ev->hw_addr = ((void*) ev) + sizeof(struct ev_nf_queue_packet_msg);
			
			for (i = 0; i < hw_addrlen-1; i++)
				sprintf(ev->hw_addr+i*3, "%02x:", hwph->hw_addr[i]);
			sprintf(ev->hw_addr+i*3, "%02x", hwph->hw_addr[hw_addrlen-1]);
			printf("addr \"%s\"\n", ev->hw_addr);
		}
		
// 		id = 0;
		ph = nfq_get_msg_packet_hdr(tb);
		if (ph) {
			ev->id = ntohl(ph->packet_id);
// 			printf("hw_protocol=0x%04x hook=%u id=%u ",
// 				  ntohs(ph->hw_protocol), ph->hook, id);
			ev->hw_protocol = ntohs(ph->hw_protocol);
		}
		
		mark = nfq_get_nfmark(tb);
		if (mark) {
			printf("mark=%u ", mark);
			ev->mark = mark;
		}
		
		ifi = nfq_get_indev(tb);
		if (ifi) {
			printf("indev=%u ", ifi);
			ev->indev = ifi;
		}
		
		ifi = nfq_get_outdev(tb);
		if (ifi) {
			printf("outdev=%u ", ifi);
			ev->outdev = ifi;
		}
		
		ifi = nfq_get_physindev(tb);
		if (ifi) {
			printf("physindev=%u ", ifi);
			ev->physindev = ifi;
		}
		
		ifi = nfq_get_physoutdev(tb);
		if (ifi) {
			printf("physoutdev=%u ", ifi);
			ev->physoutdev = ifi;
		}
		
// 		struct iphdr *iph = (struct iphdr*)payload;
// 		printf("%d %s\n", iph->protocol, inet_ntoa(*(struct in_addr *)&iph->saddr));
		
		ev->payload = ((void*) ev) + sizeof(struct ev_nf_queue_packet_msg) + hw_addrlen + 1;
		ev->payload_size = payload_size;
		memcpy(ev->payload, payload, ev->payload_size);
		
// 		struct iphdr *iph = (struct iphdr*)ev->payload;
// 		printf("%d %s\n", iph->protocol, inet_ntoa(*(struct in_addr *)&iph->saddr));
		
		event = new_event();
		event->type = NF_QUEUE_NEW_PACKET_MSG_TYPE;
		event->data = ev;
		event->data_size = data_size;
		
		add_event(new_packet_graph_msg, event);
		
		wait_on_event(event);
		
		printf("packet: %s\n", event->response? "accept":"drop");
		return nfq_set_verdict(qh, ev->id, event->response? NF_ACCEPT : NF_DROP, 0, NULL);
	} else {
	
// 	if (!new_packet_graph->tasks && !new_packet_graph_msg->tasks ) {
		int id;
		struct nfqnl_msg_packet_hdr *ph;
		
		id = 0;
		ph = nfq_get_msg_packet_hdr(tb);
		if (ph)
			id = ntohl(ph->packet_id);
		else
			return 0;
		
		// TODO default
		return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
	}
	
	return 0;
}

void *nfq_tmain(void *data) {
	struct nfq_handle *h;
	struct nfq_q_handle *qh;
	int fd, rv;
	char buf[4096] __attribute__ ((aligned));
	
	h = nfq_open();
	if (!h) {
		fprintf(stderr, "error during nfq_open()\n");
		exit(1);
	}
	
	printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
	if (nfq_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		exit(1);
	}
	
	printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
	if (nfq_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_bind_pf()\n");
		exit(1);
	}
	
	printf("binding this socket to queue '0'\n");
	qh = nfq_create_queue(h,  0, &cb, NULL);
	if (!qh) {
		fprintf(stderr, "error during nfq_create_queue()\n");
		exit(1);
	}
	
	printf("setting copy_packet mode\n");
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		exit(1);
	}
	
	fd = nfq_fd(h);
	
	while ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
		printf("pkt received\n");
		nfq_handle_packet(h, buf, rv);
	}
	
	nfq_close(h);
	
	return 0;
}

struct event_task task;
struct event_graph *rl_graph = 0;


void handle(struct event *event, void *userdata) {
	struct ev_nf_queue_packet_msg *ev;
	struct event *rl_event;
	char *s;
	size_t len;
	
	printf("rec size %zu\n", event->data_size);
	if (event->data_size < sizeof(struct ev_nf_queue_packet_msg))
		return;
	
	ev = (struct ev_nf_queue_packet_msg*) event->data;
	
// 	len = ev->hw_addr?strlen(ev->hw_addr):0+3+1;
// 	s = (char*) malloc(len);
// 	sprintf(s, "hw %s\n", ev->hw_addr);
	
	
	
	
	struct sockaddr_in source,dest;
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
}

void nf_queue_init() {
	tdata.graph = cortexd_graph;

	char **event_types = (char**) malloc(sizeof(char*)*2);
	event_types[0] = NF_QUEUE_NEW_PACKET_MSG_TYPE;
	event_types[1] = 0;
	new_eventgraph(&new_packet_graph_msg, event_types);
	
	task.handle = &handle;
// 	task.userdata
	add_task(new_packet_graph_msg, &task, 255);
	
	
	pthread_create(&pthread, NULL, nfq_tmain, &tdata);
}

void nf_queue_finish() {
	
}