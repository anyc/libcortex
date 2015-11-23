
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/netfilter.h>

#include <json/json.h>

#include "core.h"
#include "nf_queue.h"

struct nfq_thread_data {
	struct event_graph *graph;
};

static pthread_t pthread;
struct nfq_thread_data tdata;


static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
		    struct nfq_data *tb, void *data)
{
// 	int verdict;
// 	u_int32_t id = treat_pkt(nfa, &verdict); /* Treat packet */
// 	return nfq_set_verdict(qh, id, verdict, 0, NULL); /* Verdict packet */
	
	
	int id = 0;
	struct nfqnl_msg_packet_hdr *ph;
	struct nfqnl_msg_packet_hw *hwph;
	u_int32_t mark,ifi; 
	int ret;
// 	char *data;
	
	
	json_object * jobj = json_object_new_object();
	
	id = 0;
	ph = nfq_get_msg_packet_hdr(tb);
	if (ph) {
		id = ntohl(ph->packet_id);
		printf("hw_protocol=0x%04x hook=%u id=%u ",
			  ntohs(ph->hw_protocol), ph->hook, id);
	}
	
	hwph = nfq_get_packet_hw(tb);
	if (hwph) {
		int i, hlen = ntohs(hwph->hw_addrlen);
		
		printf("hw_src_addr=");
		for (i = 0; i < hlen-1; i++)
			printf("%02x:", hwph->hw_addr[i]);
		printf("%02x ", hwph->hw_addr[hlen-1]);
	}
	
	mark = nfq_get_nfmark(tb);
	if (mark)
		printf("mark=%u ", mark);
	
	ifi = nfq_get_indev(tb);
	if (ifi)
		printf("indev=%u ", ifi);
	
	ifi = nfq_get_outdev(tb);
	if (ifi)
		printf("outdev=%u ", ifi);
	
	ifi = nfq_get_physindev(tb);
	if (ifi)
		printf("physindev=%u ", ifi);
	
	ifi = nfq_get_physoutdev(tb);
	if (ifi)
		printf("physoutdev=%u ", ifi);
	
// 	ret = nfq_get_payload(tb, &data);
// 	if (ret >= 0) {
// 		printf("payload_len=%d ", ret);
// 		processPacketData (data, ret);
// 	}
// 	fputc('\n', stdout);
	printf("\n");
	
	
	
	
	
	struct event *event;
	
	listener = (struct listener_data*) userdata;
	
	event = (struct event*) malloc(sizeof(struct event));
	event->type = "nf_queue/packet";
	event->data = tb;
	
	add_raw_event(event);
	
	
	return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
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
}

void nf_queue_init() {
	tdata.graph = cortexd_graph;

	pthread_create(&pthread, NULL, nfq_tmain, &tdata);
}

void nf_queue_finish() {
	
}