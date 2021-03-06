#ifndef _CRTX_NF_QUEUE_H
#define _CRTX_NF_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

// #define NFQ_PACKET_MSG_ETYPE "nf_queue/packet_msg"

#define PFW_NEWPACKET_SIGNATURE "ussssD"
enum {PFW_NEWP_PROTOCOL=0, PFW_NEWP_SRC_IP, PFW_NEWP_SRC_HOST, PFW_NEWP_DST_IP, PFW_NEWP_DST_HOST, PFW_NEWP_PAYLOAD};

#define PFW_NEWPACKET_TCP_SIGNATURE "uu"
enum {PFW_NEWP_TCP_SPORT=0, PFW_NEWP_TCP_DPORT};

#define PFW_NEWPACKET_UDP_SIGNATURE "uu"
enum {PFW_NEWP_UDP_SPORT=0, PFW_NEWP_UDP_DPORT};


struct crtx_nf_queue_listener {
	struct crtx_listener_base base;
	
	pthread_t thread;
	
	u_int16_t queue_num;
	u_int32_t default_mark;
};

struct crtx_nfq_packet {
	int id;
	u_int16_t hw_protocol;
	
	char *hw_addr;
	
	u_int32_t mark_in;
	u_int32_t indev;
	u_int32_t outdev;
	u_int32_t physindev;
	u_int32_t physoutdev;
	
	unsigned char *payload;
	size_t payload_size;
	
	u_int32_t mark_out;
};

char crtx_nfq_print_packet(struct crtx_dict *ds);
char *crtx_nfq_proto2str(u_int16_t protocol);

char nfq_packet_msg_okay(struct crtx_event *event);
struct crtx_listener_base *crtx_setup_nf_queue_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(nf_queue)

void crtx_nf_queue_init();
void crtx_nf_queue_finish();

#ifdef __cplusplus
}
#endif

#endif
