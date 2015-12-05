
struct nfq_thread_data {
	pthread_t thread;
	
	u_int16_t queue_num;
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

char nfq_packet_msg_okay(struct event *event);
void *new_nf_queue_listener(void *options);
void nf_queue_init();
void nf_queue_finish();

char * nfq_decision_cache_create_key(struct event *event);
char *nfq_proto2str(u_int16_t protocol);