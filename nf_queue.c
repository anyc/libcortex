/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

/*
 * Setup an iptables rule like the following to use nfqueue:
 *
 * iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW  -m mark --mark 0 -j NFQUEUE --queue-num 0
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <arpa/inet.h>
#include <inttypes.h>
#include <linux/types.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/netfilter.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <sys/socket.h>

#include "intern.h"
#include "core.h"
#include "nf_queue.h"
#include "threads.h"

// char *nfq_packet_msg_etype[] = { NFQ_PACKET_MSG_ETYPE, 0 };


char crtx_nfq_print_packet(struct crtx_dict *ds) {
	struct crtx_dict *pds;
// 	char src_local, dst_local;
	
// 	ds = event->data.dict;
	
	if (strcmp(ds->signature, PFW_NEWPACKET_SIGNATURE)) {
		printf("error, signature mismatch: %s %s\n", ds->signature, PFW_NEWPACKET_SIGNATURE);
		return 1;
	}
	
// 	if (pfw_is_ip_local(ds->items[PFW_NEWP_SRC_IP].string)) {
// 		src_local = 1;
// 	} else {
// 		src_local = 0;
// 		if (pfw_is_ip_local(ds->items[PFW_NEWP_DST_IP].string))
// 			dst_local = 1;
// 		else
// 			dst_local = 0;
// 	}
	
	printf("%s ", crtx_nfq_proto2str(ds->items[PFW_NEWP_PROTOCOL].uint32));
	
// 	if (!src_local && !dst_local) {
// 		printf("brd %s %s ", ds->items[PFW_NEWP_SRC_IP].string, ds->items[PFW_NEWP_SRC_HOST].string);
// 	} else {
// 		if (src_local) {
// 			printf("dst %s %s ", ds->items[PFW_NEWP_DST_IP].string, ds->items[PFW_NEWP_DST_HOST].string);
// 		} else {
// 			printf("src %s %s ", ds->items[PFW_NEWP_SRC_IP].string, ds->items[PFW_NEWP_SRC_HOST].string);
// 		}
// 	}
	
	printf("src %s %s ", ds->items[PFW_NEWP_SRC_IP].string, ds->items[PFW_NEWP_SRC_HOST].string);
	printf("dst %s %s ", ds->items[PFW_NEWP_DST_IP].string, ds->items[PFW_NEWP_DST_HOST].string);
	
	if (ds->items[PFW_NEWP_PROTOCOL].uint32 == 6) {
		pds = ds->items[PFW_NEWP_PAYLOAD].dict;
		
		if (strcmp(pds->signature, PFW_NEWPACKET_TCP_SIGNATURE)) {
			printf("error, signature mismatch: %s %s\n", pds->signature, PFW_NEWPACKET_TCP_SIGNATURE);
			return 1;
		}
		
		printf("%u %u ", pds->items[PFW_NEWP_TCP_SPORT].uint32, pds->items[PFW_NEWP_TCP_DPORT].uint32);
	} else
	if (ds->items[PFW_NEWP_PROTOCOL].uint32 == 17) {
		pds = ds->items[PFW_NEWP_PAYLOAD].dict;
		
		if (strcmp(pds->signature, PFW_NEWPACKET_UDP_SIGNATURE)) {
			printf("error, signature mismatch: %s %s\n", pds->signature, PFW_NEWPACKET_UDP_SIGNATURE);
			return 1;
		}
		
		printf("%u %u ", pds->items[PFW_NEWP_UDP_SPORT].uint32, pds->items[PFW_NEWP_UDP_DPORT].uint32);
	}
	printf("\n");
	
	return 1;
}

static void nfq_raw2dict(struct crtx_event *event) {
	struct crtx_nfq_packet *ev;
	int res;
	char host[NI_MAXHOST];
	struct iphdr *iph;
	size_t slen;
	struct crtx_dict_item *di;
	struct crtx_dict *ds;
	struct sockaddr_in sa;
	
	
	if (event->data.size < sizeof(struct crtx_nfq_packet))
		return;
	
	VDBG("convert nfq raw2dict\n");
	
	ev = (struct crtx_nfq_packet*) event->data.pointer;
	
	iph = (struct iphdr*)ev->payload;
	sa.sin_family = AF_INET;
	
	ds = crtx_init_dict(PFW_NEWPACKET_SIGNATURE, strlen(PFW_NEWPACKET_SIGNATURE), 0);
// 	crtx_dict_upgrade_event_data(event, ds, 0);
	crtx_event_set_dict_data(event, ds, 1);
// 	event->data->dict = ds;
	di = ds->items;
	
	crtx_fill_data_item(di, 'u', "protocol", iph->protocol, 0, 0); di++;
	
	{
		slen = 0;
		crtx_fill_data_item(di, 's', "src_ip", crtx_stracpy(inet_ntoa(*(struct in_addr *)&iph->saddr), &slen), slen, 0); di++;
		
		sa.sin_addr = *(struct in_addr *)&iph->saddr;
		res = getnameinfo((struct sockaddr*) &sa, sizeof(struct sockaddr), host, 64, 0, 0, NI_NAMEREQD);
		if (res) {
			if (res != EAI_NONAME)
				printf("getnameinfo failed: %d %s\n", res, gai_strerror(res));
			host[0] = 0;
		}
		
		slen = strlen(host);
		crtx_fill_data_item(di, 's', "src_hostname", crtx_stracpy(host, &slen), slen, 0); di++;
	}
	
	{
		slen = 0;
		crtx_fill_data_item(di, 's', "dst_ip", crtx_stracpy(inet_ntoa(*(struct in_addr *)&iph->daddr), &slen), slen, 0); di++;
		
		sa.sin_addr = *(struct in_addr *)&iph->daddr;
		res = getnameinfo((struct sockaddr*) &sa, sizeof(struct sockaddr), host, 64, 0, 0, NI_NAMEREQD);
		if (res) {
			if (res != EAI_NONAME)
				printf("getnameinfo failed: %d %s\n", res, gai_strerror(res));
			host[0] = 0;
		}
		
		slen = strlen(host);
		crtx_fill_data_item(di, 's', "dst_hostname", crtx_stracpy(host, &slen), slen, 0); di++;
	}
	
	crtx_fill_data_item(di, 'D', "payload", 0, 0, CRTX_DIF_LAST_ITEM);
	
	if (iph->protocol == 6) {
		struct crtx_dict_item *pdi;
		struct tcphdr *tcp;
		
		tcp = (struct tcphdr *) (ev->payload + (iph->ihl << 2));
		
		di->dict = crtx_init_dict(PFW_NEWPACKET_TCP_SIGNATURE, strlen(PFW_NEWPACKET_TCP_SIGNATURE), 0);
		pdi = di->dict->items;
		
		crtx_fill_data_item(pdi, 'u', "src_port", ntohs(tcp->source), 0, 0); pdi++;
		
		crtx_fill_data_item(pdi, 'u', "dst_port", ntohs(tcp->dest), 0, CRTX_DIF_LAST_ITEM);
		
// 		fprintf(stdout, "TCP{sport=%u; dport=%u; seq=%u; ack_seq=%u; flags=u%ua%up%ur%us%uf%u; window=%u; urg=%u}\n",
// 				ntohs(tcp->source), ntohs(tcp->dest), ntohl(tcp->seq), ntohl(tcp->ack_seq)
// 				,tcp->urg, tcp->ack, tcp->psh, tcp->rst, tcp->syn, tcp->fin, ntohs(tcp->window), tcp->urg_ptr);
	} else
	if (iph->protocol == 17) {
		struct crtx_dict_item *pdi;
		struct udphdr *udp;
		
		udp = (struct udphdr *) (ev->payload + (iph->ihl << 2));
		
		di->dict = crtx_init_dict(PFW_NEWPACKET_UDP_SIGNATURE, strlen(PFW_NEWPACKET_UDP_SIGNATURE), 0);
		pdi = di->dict->items;
		
		crtx_fill_data_item(pdi, 'u', "src_port", ntohs(udp->source), 0, 0); pdi++;
		
		crtx_fill_data_item(pdi, 'u', "dst_port", ntohs(udp->dest), 0, CRTX_DIF_LAST_ITEM);
		
// 		fprintf(stdout,"UDP{sport=%u; dport=%u; len=%u}\n",
// 				ntohs(udp->source), ntohs(udp->dest), udp->len);
	}
	
// 	crtx_print_dict(ds);
}

static int nfq_event_cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
		    struct nfq_data *tb, void *data)
{
	struct crtx_nfq_listener *nfq_list;
	struct nfqnl_msg_packet_hdr *ph;
	struct nfqnl_msg_packet_hw *hwph;
	u_int32_t mark, ifi;
	int ret;
	
	nfq_list = (struct crtx_nfq_listener*) data;
	
	if (!is_graph_empty(nfq_list->base.graph, 0)) {
		struct crtx_nfq_packet *pkt;
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
		pkt = calloc(1, data_size);
		
		if (hwph) {
			pkt->hw_addr = ((void*) pkt) + sizeof(struct crtx_nfq_packet);
			
			memcpy(pkt->hw_addr, hwph->hw_addr, hw_addrlen*sizeof(hwph->hw_addr[0]));
			
// 			int i;
// 			for (i = 0; i < hw_addrlen-1; i++)
// 				sprintf(pkt->hw_addr+i*3, "%02x:", hwph->hw_addr[i]);
// 			sprintf(pkt->hw_addr+i*3, "%02x", hwph->hw_addr[hw_addrlen-1]);
// 			printf("addr \"%s\"\n", pkt->hw_addr);
		}
		
		ph = nfq_get_msg_packet_hdr(tb);
		if (ph) {
			pkt->id = ntohl(ph->packet_id);
// 			printf("hw_protocol=0x%04x hook=%u id=%u ", ntohs(ph->hw_protocol), ph->hook, id);
			pkt->hw_protocol = ntohs(ph->hw_protocol);
		}
		
		mark = nfq_get_nfmark(tb);
		if (mark) {
// 			printf("mark=%u ", mark);
			pkt->mark_in = mark;
		}
		
		ifi = nfq_get_indev(tb);
		if (ifi) {
// 			printf("indev=%u ", ifi);
			pkt->indev = ifi;
		}
		
		ifi = nfq_get_outdev(tb);
		if (ifi) {
// 			printf("ounfq_listev=%u ", ifi);
			pkt->outdev = ifi;
		}
		
		ifi = nfq_get_physindev(tb);
		if (ifi) {
// 			printf("physindev=%u ", ifi);
			pkt->physindev = ifi;
		}
		
		ifi = nfq_get_physoutdev(tb);
		if (ifi) {
// 			printf("physounfq_listev=%u ", ifi);
			pkt->physoutdev = ifi;
		}
		
// 		struct iphdr *iph = (struct iphdr *) payload;
// 		printf("rec: %s %s ", crtx_nfq_proto2str(iph->protocol),
// 				inet_ntoa(*(struct in_addr *)&iph->saddr));
// 		printf("%s\n", inet_ntoa(*(struct in_addr *)&iph->daddr));
		
		pkt->payload = ((void*) pkt) + sizeof(struct crtx_nfq_packet) + hw_addrlen*sizeof(hwph->hw_addr[0]);
		pkt->payload_size = payload_size;
		memcpy(pkt->payload, payload, pkt->payload_size);
		
		pkt->mark_out = nfq_list->default_mark;
		
		event = crtx_create_event(nfq_list->base.graph->types[0]); //, pkt, data_size);
// 		event->data.flags |= CRTX_DIF_DONT_FREE_DATA;
// 		event->data.to_dict = &nfq_raw2dict;
		crtx_event_set_raw_data(event, 'p', pkt, data_size, CRTX_DIF_DONT_FREE_DATA);
		
// 		crtx_dict_upgrade_event_data(event, 0, 1);
		crtx_event_set_dict_data(event, 0, 1);
		
		struct crtx_dict_item *di;
		di = crtx_get_item_by_idx(event->data.dict, 2);
		crtx_fill_data_item(di, 'p', "raw2dict", nfq_raw2dict, 0, 0);
		
		
		
		reference_event_release(event);
		
		crtx_add_event(nfq_list->base.graph, event);
		
		wait_on_event(event);
		
// 		if (event->response.raw.type == 'U')
// 			pkt->mark_out = event->response.raw.uint64;
		
		if (event->response.type != 0) {
			crtx_get_item_value(&event->response, 'u', &pkt->mark_out, sizeof(pkt->mark_out));
// 			if (!ret)
// 				pkt->mark_out = nfq_list->default_mark;
		}
		
		INFO("packet mark: %" PRIu32 "\n", pkt->mark_out);
		
		ret = nfq_set_verdict2(qh, pkt->id, NF_REPEAT, pkt->mark_out, 0, NULL);
		
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
		
		return nfq_set_verdict2(qh, id, NF_REPEAT, nfq_list->default_mark, 0, NULL);
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
	
	printf("binding nf _queue as nf_queue handler for AF_INET\n");
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

void free_nf_queue_listener(struct crtx_listener_base *data, void *userdata) {
}

struct crtx_listener_base *crtx_new_nf_queue_listener(void *options) {
	struct crtx_nfq_listener *nfq_listener;
	
	nfq_listener = (struct crtx_nfq_listener*) options;
	nfq_listener->base.free = &free_nf_queue_listener;
	
// 	new_eventgraph(&nfq_listata->base.graph, 0, nfq_packet_msg_etype);
	
// 	nfq_listata->base.thread = get_thread(nfq_tmain, nfq_listata, 0);
	nfq_listener->base.thread_job.fct = &nfq_tmain;
	nfq_listener->base.thread_job.fct_data = nfq_listener;
	nfq_listener->base.start_listener = 0;
	
	return &nfq_listener->base;
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

CRTX_DEFINE_ALLOC_FUNCTION(nf_queue)

void crtx_nf_queue_init() {
}

void crtx_nf_queue_finish() {
}
