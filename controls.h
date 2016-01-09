
// struct main_cb {
// 	void *(*create_listener)(char *id, void *options);
// 	void (*add_task)(struct crtx_graph *graph, struct crtx_task *task, unsigned char priority);
// 	struct crtx_event *(*new_event)();
// 	void (*add_event)(struct crtx_graph *graph, struct crtx_event *event);
// 	void (*wait_on_event)(struct crtx_event *event);
// 	struct crtx_graph *(*get_graph_for_event_type)(char *event_type);
// };

void controls_init();
void controls_finish();