
#define NFQ_PACKET_MSG_ETYPE "nf_queue/packet_msg"

#define PFW_NEWPACKET_SIGNATURE "ussssD"
enum {PFW_NEWP_PROTOCOL=0, PFW_NEWP_SRC_IP, PFW_NEWP_SRC_HOST, PFW_NEWP_DST_IP, PFW_NEWP_DST_HOST, PFW_NEWP_PAYLOAD};

#define PFW_NEWPACKET_TCP_SIGNATURE "uu"
enum {PFW_NEWP_TCP_SPORT=0, PFW_NEWP_TCP_DPORT};

#define PFW_NEWPACKET_UDP_SIGNATURE "uu"
enum {PFW_NEWP_UDP_SPORT=0, PFW_NEWP_UDP_DPORT};


struct crtx_nfq_listener {
	struct crtx_listener_base parent;
	
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

char nfq_packet_msg_okay(struct crtx_event *event);
struct crtx_listener_base *crtx_new_nf_queue_listener(void *options);
void crtx_nf_queue_init();
void crtx_nf_queue_finish();

char crtx_nfq_print_packet(struct crtx_dict *ds);
char *crtx_nfq_proto2str(u_int16_t protocol);