
#include <pthread.h>

#define STRINGIFYB(x) #x
#define STRINGIFY(x) STRINGIFYB(x)

#define ASSERT(x) { if (!(x)) { printf(__FILE__ ":" STRINGIFY(__LINE__) " assertion failed: " #x "\n"); exit(1); } }

#define ASSERT_STR(x, msg, ...) { if (!(x)) { printf(__FILE__ ":" STRINGIFY(__LINE__) " " msg " " #x "failed\n", __VA_ARGS__); exit(1); } }

struct event {
	char *type;
	
	void *data;
	
	struct event *next;
};

struct event_task {
	char priority;
	
	void (*handle)(struct event *event, void *userdata);
	void *userdata;
	
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
	
	struct event *equeue;
	unsigned int n_queue_entries;
	
	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_cond;
};

struct plugin {
	char *id;
	
	void (*init)();
	void (*finish)();
};

extern struct plugin static_plugins[];
extern struct event_graph *cortexd_graph;

extern struct event_graph **graphs;
extern unsigned int n_graphs;

void init_core();
void get_eventgraph(struct event_graph **event_graph, char **event_types, unsigned int n_event_types);
void add_event_type(char *event_type);
void add_raw_event(struct event *event);
void add_event(struct event_graph *graph, struct event *event);
struct event_task *new_task();