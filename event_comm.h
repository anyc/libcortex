
#include "dict_inout.h"

void write_event_as_dict(struct crtx_event *event, write_fct send, void *conn_id);
struct crtx_event *read_event_as_dict(read_fct recv, void *conn_id);

void create_in_out_box();