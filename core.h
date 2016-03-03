
#ifndef _CRTX_CORE_H
#define _CRTX_CORE_H

#include <stdint.h>
#include <regex.h>

#include "threads.h"
#include "dict.h"
#include "linkedlist.h"


#define STRINGIFYB(x) #x
#define STRINGIFY(x) STRINGIFYB(x)

#define CRTX_INFO 1<<0
#define CRTX_ERR 1<<1
#define CRTX_DBG 1<<2
#define CRTX_VDBG 1<<3

#define INFO(fmt, ...) do { crtx_printf(CRTX_INFO, fmt, ##__VA_ARGS__); } while (0)

#ifndef DEVDEBUG
	#define DBG(fmt, ...) do { crtx_printf(CRTX_DBG, fmt, ##__VA_ARGS__); } while (0)
	#define VDBG(fmt, ...) do { crtx_printf(CRTX_VDBG, fmt, ##__VA_ARGS__); } while (0)
#else
	#define DBG(fmt, ...) do { crtx_printf(CRTX_DBG, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
	#define VDBG(fmt, ...) do { crtx_printf(CRTX_VDBG, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)
#endif

#ifndef SEGV_ON_ERROR
	#define ERROR(fmt, ...) do { crtx_printf(CRTX_ERR, "\x1b[31m"); crtx_printf(CRTX_DBG, fmt, ##__VA_ARGS__); crtx_printf(CRTX_DBG, "\x1b[0m"); } while (0)
#else
	#define ERROR(fmt, ...) do { crtx_printf(CRTX_ERR, "\x1b[31m"); crtx_printf(CRTX_DBG, fmt, ##__VA_ARGS__); crtx_printf(CRTX_DBG, "\x1b[0m"); char *d = 0; printf("%c", *d); } while (0)
#endif

#define ASSERT(x) do { if (!(x)) { ERROR(__FILE__ ":" STRINGIFY(__LINE__) " assertion failed: " #x "\n"); exit(1); } } while (0)

#define ASSERT_STR(x, msg, ...) do { if (!(x)) { ERROR(__FILE__ ":" STRINGIFY(__LINE__) " " msg " " #x "failed\n", __VA_ARGS__); exit(1); } } while (0)

#define POPCOUNT32(x) __builtin_popcount(x);

#define CRTX_TEST_MAIN(mainfct) \
	int main(int argc, char **argv) { \
		int i; \
		crtx_init(); \
		crtx_handle_std_signals(); \
		i = mainfct(argc, argv); \
		crtx_finish(); \
		return i; \
	}


#define CRTX_EVT_NOTIFICATION "cortexd.notification"
extern char *crtx_evt_notification[];
#define CRTX_EVT_INBOX "cortexd.inbox"
extern char *crtx_evt_inbox[];
#define CRTX_EVT_OUTBOX "cortexd.outbox"
extern char *crtx_evt_outbox[];



struct crtx_event;

// #define CRTX_EVF_DONT_FREE_RAW 1<<0

struct crtx_event_data {
// 	void *raw;
// 	size_t raw_size;
	struct crtx_dict_item raw;
	
// 	char flags;
	
	void *(*copy_raw)(struct crtx_event_data *data);
	void (*raw_to_dict)(struct crtx_event_data *data);
	
	struct crtx_dict *dict;
};

struct crtx_event {
	char *type;
	
	struct crtx_event_data data;
	
	struct crtx_event_data response;
	char response_expected;
	
	char error;
	char claimed;
	char release_in_progress;
	
	uint64_t original_event_id;
	
	unsigned char refs_before_response;
	unsigned char refs_before_release;
	pthread_cond_t response_cond;
	pthread_cond_t release_cond;
	
// 	void (*event_to_str)(struct crtx_event *event);
	
	void (*cb_before_release)(struct crtx_event *event);
	void *cb_before_release_data;
	
	pthread_mutex_t mutex;
};

// struct crtx_event_ll {
// 	struct crtx_event *event;
// 	
// 	char in_process;
// 	
// 	struct crtx_event_ll *prev;
// 	struct crtx_event_ll *next;
// };

typedef char (*crtx_handle_task_t)(struct crtx_event *event, void *userdata, void **sessiondata);

struct crtx_graph;
struct crtx_task {
	char *id;
	unsigned char position;
	struct crtx_graph *graph;
	
	char *event_type_match; // if 0, task receives all events passing the graph
	
	crtx_handle_task_t handle;
	crtx_handle_task_t cleanup;
	void *userdata;
	
	struct crtx_task *prev;
	struct crtx_task *next;
};

#define CRTX_GRAPH_KEEP_GOING 1<<0

struct crtx_graph {
	char *name;
	
	char stop;
	unsigned char flags;
	
	char **types;
	unsigned int n_types;
	
	pthread_mutex_t mutex;
	
	struct crtx_task *tasks;
	
// 	struct crtx_thread *consumers;
	unsigned int n_consumers;
	unsigned int n_max_consumers;
	
// 	struct crtx_event_ll *equeue;
	struct crtx_dll *equeue;
// 	unsigned int n_queue_entries;
	
	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_cond;
	
	struct crtx_dll ll_hdr;
};

struct crtx_listener_repository {
	char *id;
	
	struct crtx_listener_base *(*create)(void *options);
};

struct crtx_listener_base {
	void (*free)(struct crtx_listener_base *data);
// 	struct crtx_signal finished;
	
	// TODO: add (*start)() to start listener after tasks have been added?
	
	struct crtx_thread *thread;
	char (*start_listener)(struct crtx_listener_base *listener);
	
	struct crtx_graph *graph;
};

struct crtx_module {
	char *id;
	
	void (*init)();
	void (*finish)();
};

struct crtx_epoll_listener;
struct crtx_root {
	struct crtx_graph **graphs;
	unsigned int n_graphs;
	MUTEX_TYPE graphs_mutex;
	
	struct crtx_dll *graph_queue;
	MUTEX_TYPE graph_queue_mutex;
	
	char shutdown;
	
	struct crtx_epoll_listener *epoll_listener;
	
	struct crtx_graph *crtx_ctrl_graph;
	
	void *notification_listeners_handle;
};

extern struct crtx_module static_modules[];
// extern struct crtx_graph *cortexd_graph;

// extern struct crtx_graph **graphs;
// extern unsigned int n_graphs;

#define CRTX_EVT_SHUTDOWN "cortexd.control.shutdown"
#define CRTX_EVT_MOD_INIT "cortex.control.module_initialized"
// extern struct crtx_graph *crtx_ctrl_graph;
extern struct crtx_root *crtx_root;


void crtx_printf(char level, char *format, ...);
char *stracpy(const char *str, size_t *str_length);
struct crtx_event *new_event();
void free_event(struct crtx_event *event);
void crtx_init();
void crtx_finish();
void crtx_loop();
void new_eventgraph(struct crtx_graph **crtx_graph, char *name, char **event_types);
void free_eventgraph(struct crtx_graph *egraph);
// void get_eventgraph(struct crtx_graph **crtx_graph, char **event_types, unsigned int n_event_types);
void add_task(struct crtx_graph *graph, struct crtx_task *task);
void add_event_type(char *event_type);
void add_raw_event(struct crtx_event *event);
void add_event(struct crtx_graph *graph, struct crtx_event *event);
void add_event_sync(struct crtx_graph *graph, struct crtx_event *event);
struct crtx_graph *find_graph_for_event_type(char *event_type);
struct crtx_graph *find_graph_by_name(char *name);
struct crtx_graph *get_graph_for_event_type(char *event_type, char **new_event_types);
struct crtx_task *new_task();
void free_task(struct crtx_task *task);
char wait_on_event(struct crtx_event *event);
struct crtx_listener_base *create_listener(char *id, void *options);
void free_listener(struct crtx_listener_base *listener);
struct crtx_event *create_event(char *type, void *data, size_t data_size);
struct crtx_task *crtx_create_task(struct crtx_graph *graph, unsigned char position, char *id, crtx_handle_task_t handler, void *userdata);
void crtx_claim_next_event(struct crtx_dll **graph, struct crtx_dll **event);
void crtx_process_event(struct crtx_graph *graph, struct crtx_dll *queue_entry);
void crtx_init_shutdown();

char crtx_start_listener(struct crtx_listener_base *listener);
void print_tasks(struct crtx_graph *graph);
void hexdump(unsigned char *buffer, size_t index);

void crtx_traverse_graph(struct crtx_graph *graph, struct crtx_event *event);
void reference_event_response(struct crtx_event *event);
void dereference_event_response(struct crtx_event *event);
void reference_event_release(struct crtx_event *event);
void dereference_event_release(struct crtx_event *event);

// void event_ll_add(struct crtx_event_ll **list, struct crtx_event *event);
char *get_username();
char is_graph_empty(struct crtx_graph *graph, char *event_type);
void *crtx_copy_raw_data(struct crtx_event_data *data);

void crtx_init_notification_listeners(void **data);
void crtx_finish_notification_listeners(void *data);

void crtx_handle_std_signals();

#endif
