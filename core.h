
#include <stdint.h>
#include <pthread.h>
#include <regex.h>

#define STRINGIFYB(x) #x
#define STRINGIFY(x) STRINGIFYB(x)

#define ASSERT(x) { if (!(x)) { printf(__FILE__ ":" STRINGIFY(__LINE__) " assertion failed: " #x "\n"); exit(1); } }

#define ASSERT_STR(x, msg, ...) { if (!(x)) { printf(__FILE__ ":" STRINGIFY(__LINE__) " " msg " " #x "failed\n", __VA_ARGS__); exit(1); } }

#define CRTX_EVT_NOTIFICATION "cortexd/notification"
extern char *crtx_evt_notification[];
#define CRTX_EVT_INBOX "cortexd/inbox"
extern char *crtx_evt_inbox[];
#define CRTX_EVT_OUTBOX "cortexd/outbox"
extern char *crtx_evt_outbox[];

struct event;

struct signal {
	char *condition;
	char local_condition;
	
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

struct event_data {
	void *raw;
	size_t raw_size;
	
	void (*raw_to_dict)(struct event *event);
	
	struct crtx_dict *dict;
};

struct event {
	char *type;
	
	struct event_data data;
	
	char response_expected;
	struct event_data response;
	
// 	void (*respond)(struct event *to_event, struct event_data *ev);
// 	void *respond_cb_payload;
	uint64_t original_event_id;
// 	struct event *original_event;
	
	unsigned char refs_before_response;
	unsigned char refs_before_release;
	pthread_cond_t response_cond;
// 	pthread_cond_t release_cond;
	void (*cb_before_release)(struct event *event);
	void *cb_before_release_data;
	
	pthread_mutex_t mutex;
};

struct queue_entry {
	struct event *event;
	
	struct queue_entry *next;
};

struct event_graph;
struct event_task {
	char *id;
	unsigned char position;
	struct event_graph *graph;
	
	void (*handle)(struct event *event, void *userdata, void **sessiondata);
	void (*cleanup)(struct event *event, void *userdata, void *sessiondata);
	void *userdata;
	
	struct event_task *prev;
	struct event_task *next;
};

struct thread {
	pthread_t handle;
	
	char stop;
	
	struct event_graph *graph;
};

struct event_graph {
	char **types;
	unsigned int n_types;
	
	pthread_mutex_t mutex;
	
	struct event_task *tasks;
	
	struct thread *consumers;
	unsigned int n_consumers;
	
	struct queue_entry *equeue;
	unsigned int n_queue_entries;
	
	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_cond;
};

struct listener_factory {
	char *id;
	
	struct listener *(*create)(void *options);
};

struct listener {
	void (*free)(void *data);
	
	struct event_graph *graph;
};

struct module {
	char *id;
	
	void (*init)();
	void (*finish)();
};

#define DIF_KEY_ALLOCATED 1<<0
#define DIF_DATA_UNALLOCATED 1<<1
#define DIF_LAST 1<<7

#define DIF_IS_LAST(di) ( ((di)->flags & DIF_LAST) != 0 )

struct crtx_dict_item {
	char *key;
	unsigned long size;
	
	char type;
	char flags;
	
	union {
		char *string; // s
		uint32_t uint32; // u
		int32_t int32; // i
		uint64_t uint64; // z
		void *pointer; // p
		struct crtx_dict *ds; // D
// 		char payload[0]; // P
	};
};

struct crtx_dict {
	char *signature;
	unsigned char signature_length;
	unsigned long size;
	
	struct crtx_dict_item items[0];
};

extern struct module static_modules[];
extern struct event_graph *cortexd_graph;

extern struct event_graph **graphs;
extern unsigned int n_graphs;




char *stracpy(char *str, size_t *str_length);
struct event *new_event();
void free_event(struct event *event);
void cortex_init();
void cortex_finish();
void new_eventgraph(struct event_graph **event_graph, char **event_types);
void free_eventgraph(struct event_graph *egraph);
// void get_eventgraph(struct event_graph **event_graph, char **event_types, unsigned int n_event_types);
void add_task(struct event_graph *graph, struct event_task *task);
void add_event_type(char *event_type);
void add_raw_event(struct event *event);
void add_event(struct event_graph *graph, struct event *event);
void add_event_sync(struct event_graph *graph, struct event *event);
struct event_graph *find_graph_for_event_type(char *event_type);
struct event_graph *get_graph_for_event_type(char *event_type, char **event_types);
struct event_task *new_task();
void free_task(struct event_task *task);
void wait_on_event(struct event *event);
struct listener *create_listener(char *id, void *options);
void free_listener(struct listener *listener);
struct event *create_event(char *type, void *data, size_t data_size);
// struct event *create_response(struct event *event, void *data, size_t data_size);

void print_tasks(struct event_graph *graph);
void hexdump(unsigned char *buffer, size_t index);

struct crtx_dict * crtx_create_dict(char *signature, ...);
void free_dict(struct crtx_dict *ds);
char crtx_get_value(struct crtx_dict *ds, char *key, void *buffer, size_t buffer_size);
char crtx_copy_value(struct crtx_dict_item *di, void *buffer, size_t buffer_size);
void crtx_print_dict(struct crtx_dict *ds);
struct crtx_dict_item *crtx_get_first_item(struct crtx_dict *ds);
struct crtx_dict_item *crtx_get_next_item(struct crtx_dict_item *di);

struct signal *new_signal();
void init_signal(struct signal *signal);
void wait_on_signal(struct signal *s);
void send_signal(struct signal *s, char brdcst);
void free_signal(struct signal *s);

void crtx_traverse_graph(struct event_graph *graph, struct event *event);
void reference_event_response(struct event *event);
void dereference_event_response(struct event *event);
void reference_event_release(struct event *event);
void dereference_event_release(struct event *event);

void event_ll_add(struct queue_entry **list, struct event *event);