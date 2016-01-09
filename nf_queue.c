
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

#define NFQ_PACKET_MSG_ETYPE "nf_queue/packet_msg"
char *nfq_packet_msg_etype[] = { NFQ_PACKET_MSG_ETYPE, 0 };

// iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW  -m mark --mark 0 -j NFQUEUE --queue-num 0

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
		    struct nfq_data *tb, void *data)
{
	struct nfqnl_msg_packet_hdr *ph;
	struct nfqnl_msg_packet_hw *hwph;
	u_int32_t mark,ifi; 
	
	struct nfq_thread_data *td = (struct nfq_thread_data*) data;
	
	struct crtx_graph *new_packet_graph_msg = td->parent.graph;
	
	if (new_packet_graph_msg && new_packet_graph_msg->tasks) {
		struct ev_nf_queue_packet_msg *ev;
		struct crtx_event *event;
		size_t msg_size;
		unsigned char *payload;
		size_t payload_size, data_size;
		u_int16_t hw_addrlen;
		
		
		msg_size = 0;
		
		hwph = nfq_get_packet_hw(tb);
		if (hwph) {
			hw_addrlen = ntohs(hwph->hw_addrlen);
			msg_size += hw_addrlen*sizeof(hwph->hw_addr[0]);
		} else
			hw_addrlen = 0;
		
		payload_size = nfq_get_payload(tb, &payload);
		msg_size += payload_size;
		
		data_size = msg_size + sizeof(struct ev_nf_queue_packet_msg);
		ev = calloc(1, data_size);
		
		if (hwph) {
			ev->hw_addr = ((void*) ev) + sizeof(struct ev_nf_queue_packet_msg);
			
			memcpy(ev->hw_addr, hwph->hw_addr, hw_addrlen*sizeof(hwph->hw_addr[0]));
			
// 			int i;
// 			for (i = 0; i < hw_addrlen-1; i++)
// 				sprintf(ev->hw_addr+i*3, "%02x:", hwph->hw_addr[i]);
// 			sprintf(ev->hw_addr+i*3, "%02x", hwph->hw_addr[hw_addrlen-1]);
// 			printf("addr \"%s\"\n", ev->hw_addr);
		}
		
		ph = nfq_get_msg_packet_hdr(tb);
		if (ph) {
			ev->id = ntohl(ph->packet_id);
// 			printf("hw_protocol=0x%04x hook=%u id=%u ", ntohs(ph->hw_protocol), ph->hook, id);
			ev->hw_protocol = ntohs(ph->hw_protocol);
		}
		
		mark = nfq_get_nfmark(tb);
		if (mark) {
// 			printf("mark=%u ", mark);
			ev->mark = mark;
		}
		
		ifi = nfq_get_indev(tb);
		if (ifi) {
// 			printf("indev=%u ", ifi);
			ev->indev = ifi;
		}
		
		ifi = nfq_get_outdev(tb);
		if (ifi) {
// 			printf("outdev=%u ", ifi);
			ev->outdev = ifi;
		}
		
		ifi = nfq_get_physindev(tb);
		if (ifi) {
// 			printf("physindev=%u ", ifi);
			ev->physindev = ifi;
		}
		
		ifi = nfq_get_physoutdev(tb);
		if (ifi) {
// 			printf("physoutdev=%u ", ifi);
			ev->physoutdev = ifi;
		}
		
// 		struct iphdr *iph = (struct iphdr *) payload;
// 		printf("rec: %s %s ", nfq_proto2str(iph->protocol),
// 				inet_ntoa(*(struct in_addr *)&iph->saddr));
// 		printf("%s\n", inet_ntoa(*(struct in_addr *)&iph->daddr));
		
		
		ev->payload = ((void*) ev) + sizeof(struct ev_nf_queue_packet_msg) + hw_addrlen*sizeof(hwph->hw_addr[0]);
		ev->payload_size = payload_size;
		memcpy(ev->payload, payload, ev->payload_size);
		
// 		event = new_event();
// 		event->type = td->parent.graph->types[0];
// 		event->data = ev;
// 		event->data_size = data_size;
		event = create_event(td->parent.graph->types[0], ev, data_size);
		
		add_event(new_packet_graph_msg, event);
		
		wait_on_event(event);
		
		printf("packet mark: %lu\n", (long) event->response.raw);
		
		int ret = nfq_set_verdict2(qh, ev->id, NF_REPEAT, (long) event->response.raw, 0, NULL);
		
		free(ev);
		free_event(event);
		
		return ret;
	} else {
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
	
	struct nfq_thread_data *td = (struct nfq_thread_data*) data;
	
	h = nfq_open();
	if (!h) {
		fprintf(stderr, "error during nfq_open()\n");
		return 0;
	}
	
	printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
	if (nfq_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		return 0;
	}
	
	printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
	if (nfq_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_bind_pf()\n");
		return 0;
	}
	
	printf("binding this socket to queue '0'\n");
	qh = nfq_create_queue(h, td->queue_num, &cb, td);
	if (!qh) {
		fprintf(stderr, "error during nfq_create_queue()\n");
		return 0;
	}
	
	printf("setting copy_packet mode\n");
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		return 0;
	}
	
	fd = nfq_fd(h);
	
	while ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0) {
		nfq_handle_packet(h, buf, rv);
	}
	
	nfq_close(h);
	
	return 0;
}

struct listener *new_nf_queue_listener(void *options) {
	struct nfq_thread_data *tdata;
	
	tdata = (struct nfq_thread_data*) malloc(sizeof(struct nfq_thread_data));
	tdata->queue_num = 0;
	tdata->parent.free = &free_nf_queue_listener;
	
// 	char **event_types = (char**) malloc(sizeof(char*)*2);
// 	event_types[0] = "nf_queue/packet_msg";
// 	event_types[1] = 0;
	new_eventgraph(&tdata->parent.graph, nfq_packet_msg_etype);
	
	
	pthread_create(&tdata->thread, NULL, nfq_tmain, tdata);
	
	return &tdata->parent;
}

void free_nf_queue_listener(void *data) {
	struct nfq_thread_data *tdata = (struct nfq_thread_data *) data;
	
	free_eventgraph(tdata->parent.graph);
	
// 	pthread_join(tdata->thread, 0);
	
	free(tdata);
}

char nfq_packet_msg_okay(struct crtx_event *event) {
	if (event->data.raw_size < sizeof(struct ev_nf_queue_packet_msg))
		return 0;
	
	return 1;
}

char *nfq_proto2str(u_int16_t protocol) {
	switch (protocol) {
		case 1: return "ICMP"; break;
		case 2: return "IGMP"; break;
		case 6: return "TCP"; break;
		case 17: return "UDP"; break;
		default: printf("unknown proto %d\n", protocol); return "unknown"; break;
	}
}

void nf_queue_init() {
}

void nf_queue_finish() {
}