
#include <pthread.h>

#define STRINGIFYB(x) #x
#define STRINGIFY(x) STRINGIFYB(x)

#define ASSERT(x) { if (!(x)) { printf(__FILE__ ":" STRINGIFY(__LINE__) " assertion failed: " #x "\n"); exit(1); } }

#define ASSERT_STR(x, msg, ...) { if (!(x)) { printf(__FILE__ ":" STRINGIFY(__LINE__) " " msg " " #x "failed\n", __VA_ARGS__); exit(1); } }

struct event {
	char *type;
	struct event_graph *graph;
	
	void *data;
	size_t data_size;
// 	char data_is_self_contained;
	
	void *response;
	size_t response_size;
// 	char response_is_self_contained;
	
	unsigned char in_n_queues;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

struct queue_entry {
	struct event *event;
	
	struct queue_entry *next;
};

struct event_task {
	char *id;
	char position;
	
	void (*handle)(struct event *event, void *userdata);
	void (*cleanup)(struct event *event, void *userdata);
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

struct listener {
	char *id;
	
	void *(*create)(void *options);
};

struct module {
	char *id;
	
	void (*init)();
	void (*finish)();
};

struct response_cache_entry {
	char *key;
	
	void *response;
	size_t response_size;
};

typedef char *(*create_key_cb_t)(struct event *event);

struct response_cache {
	struct response_cache_entry *entries;
	size_t n_entries;
	
	struct response_cache_entry *cur;
	char *cur_key;
	
	create_key_cb_t create_key;
};


extern struct module static_modules[];
extern struct event_graph *cortexd_graph;

extern struct event_graph **graphs;
extern unsigned int n_graphs;

struct event *new_event();
void init_core();
void new_eventgraph(struct event_graph **event_graph, char **event_types);
void get_eventgraph(struct event_graph **event_graph, char **event_types, unsigned int n_event_types);
void add_task(struct event_graph *graph, struct event_task *task);
void add_event_type(char *event_type);
void add_raw_event(struct event *event);
void add_event(struct event_graph *graph, struct event *event);
void add_event_sync(struct event_graph *graph, struct event *event);
struct event_graph *get_graph_for_event_type(char *event_type);
struct event_task *new_task();
void wait_on_event(struct event *event);
void *create_listener(char *id, void *options);

struct event_task *create_response_cache_task(create_key_cb_t create_key);
void print_tasks(struct event_graph *graph);
void hexdump(unsigned char *buffer, size_t index);