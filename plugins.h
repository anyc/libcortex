
struct main_cb {
	void *(*create_listener)(char *id, void *options);
	void (*add_task)(struct event_graph *graph, struct event_task *task, unsigned char priority);
	struct event *(*new_event)();
	void (*add_event)(struct event_graph *graph, struct event *event);
	void (*wait_on_event)(struct event *event);
	struct event_graph *(*get_graph_for_event_type)(char *event_type);
};

void plugins_init();
void plugins_finish();