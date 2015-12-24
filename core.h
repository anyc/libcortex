
#include <stdint.h>
#include <pthread.h>
#include <regex.h>

#define STRINGIFYB(x) #x
#define STRINGIFY(x) STRINGIFYB(x)

#define ASSERT(x) { if (!(x)) { printf(__FILE__ ":" STRINGIFY(__LINE__) " assertion failed: " #x "\n"); exit(1); } }

#define ASSERT_STR(x, msg, ...) { if (!(x)) { printf(__FILE__ ":" STRINGIFY(__LINE__) " " msg " " #x "failed\n", __VA_ARGS__); exit(1); } }

struct signal {
	char *condition;
	char local_condition;
	
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

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

struct event_graph;
struct event_task {
	char *id;
	char position;
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

struct response_cache_entry {
	void *key;
	union {
		regex_t key_regex;
	};
	union {
		char regex_initialized;
	};
	char type;
	
	void *response;
	size_t response_size;
};

struct response_cache;
typedef char *(*create_key_cb_t)(struct event *event);
typedef char (*match_cb_t)(struct response_cache *rc, struct event *event, struct response_cache_entry *c_entry);

struct response_cache {
	char *signature;
	
	struct response_cache_entry *entries;
	size_t n_entries;
	
	struct response_cache_entry *cur;
	void *cur_key;
	
	create_key_cb_t create_key;
	match_cb_t match_event;
};

#define DIF_KEY_ALLOCATED 1<<0
#define DIF_DATA_UNALLOCATED 1<<1
#define DIF_LAST 1<<7

#define DIF_IS_LAST(di) ( ((di)->flags & DIF_LAST) != 0 )

struct data_item {
	char type;
	size_t size;
	char *key;
	
	char flags;
	
	union {
		char *string;
		uint32_t uint32;
		int32_t int32;
		void *structure;
		struct data_struct *ds;
	};
};

struct data_struct {
	char *signature;
	
	struct data_item items[0];
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
void get_eventgraph(struct event_graph **event_graph, char **event_types, unsigned int n_event_types);
void add_task(struct event_graph *graph, struct event_task *task);
void add_event_type(char *event_type);
void add_raw_event(struct event *event);
void add_event(struct event_graph *graph, struct event *event);
void add_event_sync(struct event_graph *graph, struct event *event);
struct event_graph *get_graph_for_event_type(char *event_type);
struct event_task *new_task();
void free_task(struct event_task *task);
void wait_on_event(struct event *event);
struct listener *create_listener(char *id, void *options);
void free_listener(struct listener *listener);

struct event_task *create_response_cache_task(char *signature, create_key_cb_t create_key);
void print_tasks(struct event_graph *graph);
void hexdump(unsigned char *buffer, size_t index);

void free_data_struct(struct data_struct *ds);

struct signal *new_signal();
void init_signal(struct signal *signal);
void wait_on_signal(struct signal *s);
void send_signal(struct signal *s, char brdcst);
void free_signal(struct signal *s);

void free_response_cache(struct response_cache *dc);
void prefill_rcache(struct response_cache *rcache, char *file);
char rcache_match_cb_t_regex(struct response_cache *rc, struct event *event, struct response_cache_entry *c_entry);