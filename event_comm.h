

typedef int (*send_fct)(void *conn_id, void *data, size_t data_size);
typedef int (*recv_fct)(void *conn_id, void *data, size_t data_size);

#define CRTX_MAX_SIGNATURE_LENGTH 128


struct serialized_dict_item {
	uint64_t key_length;
	uint64_t payload_size;
	
	uint8_t type;
	uint8_t flags;
};

struct serialized_dict {
	uint8_t signature_length;
};

// #define CRTX_SREQ_RAW 1<<0
// #define CRTX_SREQ_DICT 1<<1

struct serialized_event {
	uint8_t version;
	uint64_t id;
	
	uint32_t type_length;
	uint8_t flags;
};


void send_event_as_dict(struct event *event, send_fct send, void *conn_id);
struct event *recv_event_as_dict(recv_fct recv, void *conn_id);

void create_in_out_box();