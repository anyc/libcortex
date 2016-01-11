
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

static int nfq_event_cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
		    struct nfq_data *tb, void *data)
{
	struct crtx_nfq_listener *nfq_list;
	struct nfqnl_msg_packet_hdr *ph;
	struct nfqnl_msg_packet_hw *hwph;
	u_int32_t mark, ifi;
	
	
	nfq_list = (struct crtx_nfq_listener*) data;
	
	if (!is_graph_empty(nfq_list->parent.graph, 0)) {
		struct crtx_nfq_packet *ev;
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
		
		data_size = msg_size + sizeof(struct crtx_nfq_packet);
		ev = calloc(1, data_size);
		
		if (hwph) {
			ev->hw_addr = ((void*) ev) + sizeof(struct crtx_nfq_packet);
			
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
// 			printf("ounfq_listev=%u ", ifi);
			ev->outdev = ifi;
		}
		
		ifi = nfq_get_physindev(tb);
		if (ifi) {
// 			printf("physindev=%u ", ifi);
			ev->physindev = ifi;
		}
		
		ifi = nfq_get_physoutdev(tb);
		if (ifi) {
// 			printf("physounfq_listev=%u ", ifi);
			ev->physoutdev = ifi;
		}
		
// 		struct iphdr *iph = (struct iphdr *) payload;
// 		printf("rec: %s %s ", crtx_nfq_proto2str(iph->protocol),
// 				inet_ntoa(*(struct in_addr *)&iph->saddr));
// 		printf("%s\n", inet_ntoa(*(struct in_addr *)&iph->daddr));
		
		
		ev->payload = ((void*) ev) + sizeof(struct crtx_nfq_packet) + hw_addrlen*sizeof(hwph->hw_addr[0]);
		ev->payload_size = payload_size;
		memcpy(ev->payload, payload, ev->payload_size);
		
		event = create_event(nfq_list->parent.graph->types[0], ev, data_size);
		
		reference_event_release(event);
		
		add_event(nfq_list->parent.graph, event);
		
		wait_on_event(event);
		
		printf("packet mark: %lu\n", (long) event->response.raw);
		
		int ret = nfq_set_verdict2(qh, ev->id, NF_REPEAT, (long) event->response.raw, 0, NULL);
		
		free(ev);
		dereference_event_release(event);
		
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
	
	struct crtx_nfq_listener *nfq_list = (struct crtx_nfq_listener*) data;
	
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
	qh = nfq_create_queue(h, nfq_list->queue_num, &nfq_event_cb, nfq_list);
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

void free_nf_queue_listener(void *data) {
	struct crtx_nfq_listener *nfq_listata = (struct crtx_nfq_listener *) data;
	
	free_eventgraph(nfq_listata->parent.graph);
	
// 	pthread_join(nfq_listata->thread, 0);
	
	free(nfq_listata);
}

struct crtx_listener_base *crtx_new_nf_queue_listener(void *options) {
	struct crtx_nfq_listener *nfq_listata;
	
// 	nfq_listata = (struct crtx_nfq_listener*) malloc(sizeof(struct crtx_nfq_listener));
// 	nfq_listata->queue_num = 0;
	
	nfq_listata = (struct crtx_nfq_listener*) options;
	nfq_listata->parent.free = &free_nf_queue_listener;
	
	new_eventgraph(&nfq_listata->parent.graph, nfq_packet_msg_etype);
	
	pthread_create(&nfq_listata->thread, NULL, nfq_tmain, nfq_listata);
	
	return &nfq_listata->parent;
}

char *crtx_nfq_proto2str(u_int16_t protocol) {
	switch (protocol) {
		case 1: return "ICMP"; break;
		case 2: return "IGMP"; break;
		case 6: return "TCP"; break;
		case 17: return "UDP"; break;
		default: printf("unknown proto %d\n", protocol); return "unknown"; break;
	}
}

void crtx_nf_queue_init() {
}

void crtx_nf_queue_finish() {
}