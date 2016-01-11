
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

struct crtx_event;

struct crtx_signal {
	char *condition;
	char local_condition;
	
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

struct crtx_event_data {
	void *raw;
	size_t raw_size;
	
	void (*raw_to_dict)(struct crtx_event *event);
	
	struct crtx_dict *dict;
};

struct crtx_event {
	char *type;
	
	struct crtx_event_data data;
	
	struct crtx_event_data response;
	char response_expected;
	
	uint64_t original_event_id;
	
	unsigned char refs_before_response;
	unsigned char refs_before_release;
	pthread_cond_t response_cond;
	
	void (*cb_before_release)(struct crtx_event *event);
	void *cb_before_release_data;
	
	pthread_mutex_t mutex;
};

struct crtx_event_ll {
	struct crtx_event *event;
	
	struct crtx_event_ll *next;
};

struct crtx_graph;
struct crtx_task {
	char *id;
	unsigned char position;
	struct crtx_graph *graph;
	
	void (*handle)(struct crtx_event *event, void *userdata, void **sessiondata);
	void (*cleanup)(struct crtx_event *event, void *userdata, void *sessiondata);
	void *userdata;
	
	struct crtx_task *prev;
	struct crtx_task *next;
};

struct crtx_thread {
	pthread_t handle;
	
	char stop;
	
	struct crtx_graph *graph;
};

struct crtx_graph {
	char **types;
	unsigned int n_types;
	
	pthread_mutex_t mutex;
	
	struct crtx_task *tasks;
	
	struct crtx_thread *consumers;
	unsigned int n_consumers;
	
	struct crtx_event_ll *equeue;
	unsigned int n_queue_entries;
	
	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_cond;
};

struct crtx_listener_repository {
	char *id;
	
	struct crtx_listener_base *(*create)(void *options);
};

struct crtx_listener_base {
	void (*free)(void *data);
	
	struct crtx_graph *graph;
};

struct crtx_module {
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

extern struct crtx_module static_modules[];
extern struct crtx_graph *cortexd_graph;

extern struct crtx_graph **graphs;
extern unsigned int n_graphs;




char *stracpy(char *str, size_t *str_length);
struct crtx_event *new_event();
void free_event(struct crtx_event *event);
void cortex_init();
void cortex_finish();
void new_eventgraph(struct crtx_graph **crtx_graph, char **event_types);
void free_eventgraph(struct crtx_graph *egraph);
// void get_eventgraph(struct crtx_graph **crtx_graph, char **event_types, unsigned int n_event_types);
void add_task(struct crtx_graph *graph, struct crtx_task *task);
void add_event_type(char *event_type);
void add_raw_event(struct crtx_event *event);
void add_event(struct crtx_graph *graph, struct crtx_event *event);
void add_event_sync(struct crtx_graph *graph, struct crtx_event *event);
struct crtx_graph *find_graph_for_event_type(char *event_type);
struct crtx_graph *get_graph_for_event_type(char *event_type, char **event_types);
struct crtx_task *new_task();
void free_task(struct crtx_task *task);
void wait_on_event(struct crtx_event *event);
struct crtx_listener_base *create_listener(char *id, void *options);
void free_listener(struct crtx_listener_base *listener);
struct crtx_event *create_event(char *type, void *data, size_t data_size);
// struct crtx_event *create_response(struct crtx_event *event, void *data, size_t data_size);

void print_tasks(struct crtx_graph *graph);
void hexdump(unsigned char *buffer, size_t index);

struct crtx_dict * crtx_create_dict(char *signature, ...);
void free_dict(struct crtx_dict *ds);
char crtx_get_value(struct crtx_dict *ds, char *key, void *buffer, size_t buffer_size);
char crtx_copy_value(struct crtx_dict_item *di, void *buffer, size_t buffer_size);
void crtx_print_dict(struct crtx_dict *ds);
struct crtx_dict_item *crtx_get_first_item(struct crtx_dict *ds);
struct crtx_dict_item *crtx_get_next_item(struct crtx_dict_item *di);

struct crtx_signal *new_signal();
void init_signal(struct crtx_signal *signal);
void wait_on_signal(struct crtx_signal *s);
void send_signal(struct crtx_signal *s, char brdcst);
void free_signal(struct crtx_signal *s);

void crtx_traverse_graph(struct crtx_graph *graph, struct crtx_event *event);
void reference_event_response(struct crtx_event *event);
void dereference_event_response(struct crtx_event *event);
void reference_event_release(struct crtx_event *event);
void dereference_event_release(struct crtx_event *event);

void event_ll_add(struct crtx_event_ll **list, struct crtx_event *event);
char *get_username();
char is_graph_empty(struct crtx_graph *graph, char *event_type);