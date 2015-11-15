
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
	void (*handle)(struct event *event);
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
	
	struct event_task *entry_task;
	
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

void init_core();
void new_eventgraph(struct event_graph **event_graph, char **event_types);
void add_event_type(char *event_type);
void add_event(struct event_graph *graph, struct event *event);