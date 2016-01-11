
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

char *crtx_nfq_proto2str(u_int16_t protocol);