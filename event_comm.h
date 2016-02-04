

typedef int (*send_fct)(void *conn_id, void *data, size_t data_size);
typedef int (*recv_fct)(void *conn_id, void *data, size_t data_size);

#define CRTX_MAX_SIGNATURE_LENGTH 128

void send_event_as_dict(struct crtx_event *event, send_fct send, void *conn_id);
struct crtx_event *recv_event_as_dict(recv_fct recv, void *conn_id);
struct crtx_dict *recv_dict(recv_fct recv, void *conn_id);

int crtx_wrapper_read(void *conn_id, void *data, size_t data_size);
int crtx_wrapper_write(void *conn_id, void *data, size_t data_size);

void create_in_out_box();