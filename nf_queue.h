
struct nfq_thread_data {
	struct crtx_listener_base parent;
	
	pthread_t thread;
	
	u_int16_t queue_num;
};

struct ev_nf_queue_packet {
	struct nfq_q_handle *qh;
	struct nfq_data *data;
};

struct ev_nf_queue_packet_msg {
	int id;
	u_int16_t hw_protocol;
	
	char *hw_addr;
	
	u_int32_t mark;
	u_int32_t indev;
	u_int32_t outdev;
	u_int32_t physindev;
	u_int32_t physoutdev;
	
	unsigned char *payload;
	size_t payload_size;
};

char nfq_packet_msg_okay(struct crtx_event *event);
struct crtx_listener_base *new_nf_queue_listener(void *options);
void free_nf_queue_listener(void *data);
void nf_queue_init();
void nf_queue_finish();

char * nfq_decision_cache_create_key(struct crtx_event *event);
char *nfq_proto2str(u_int16_t protocol);