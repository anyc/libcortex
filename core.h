#ifndef _CRTX_CORE_H
#define _CRTX_CORE_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdint.h>
#include <regex.h>

#include "threads.h"
#include "dict.h"
#include "linkedlist.h"
#include "evloop.h"

#define CRTX_TEST_MAIN(mainfct) \
	int main(int argc, char **argv) { \
		int i; \
		crtx_init(); \
		crtx_handle_std_signals(); \
		DBG("entering test function\n"); \
		i = mainfct(argc, argv); \
		DBG("left test function\n"); \
		crtx_finish(); \
		return i; \
	}

#define CRTX_RET_GEZ(r) { if (r < 0) {ERROR("%s:%d: %d %s\n", __FILE__, __LINE__, r, strerror(r)); return(r); } }

#define CRTX_EVT_NOTIFICATION "cortexd.notification"
extern char *crtx_evt_notification[];
#define CRTX_EVT_INBOX "cortexd.inbox"
extern char *crtx_evt_inbox[];
#define CRTX_EVT_OUTBOX "cortexd.outbox"
extern char *crtx_evt_outbox[];


enum crtx_processing_mode {CRTX_PREFER_NONE=0, CRTX_PREFER_THREAD, CRTX_PREFER_ELOOP, CRTX_NO_PROCESSING_MODE};


struct crtx_event;
struct crtx_listener_base;

typedef void (*crtx_raw_to_dict_t)(struct crtx_event *event, struct crtx_dict_item *item, void *user_data);

#ifndef CRTX_EVENT_TYPE_VARTYPE
#define CRTX_EVENT_TYPE_VARTYPE unsigned int
#endif

#define CRTX_EVENT_TYPE_FAMILY_MAX (2)

struct crtx_event {
	CRTX_EVENT_TYPE_VARTYPE type;
	char *description;
	
	struct crtx_dict_item data;
	
	struct crtx_dict_item response;
	char response_expected;
	
	char error;
	char claimed;
	char release_in_progress;
	
	uint64_t original_event_id;
	
	unsigned char refs_before_response;
	unsigned char refs_before_release;
	pthread_cond_t response_cond;
	pthread_cond_t release_cond;
	
	struct crtx_listener_base *origin;
	
	void (*cb_before_release)(struct crtx_event *event, void *cb_before_release_data);
	void *cb_before_release_data;
	
	pthread_mutex_t mutex;
};

typedef char (*crtx_handle_task_t)(struct crtx_event *event, void *userdata, void **sessiondata);

struct crtx_graph;
struct crtx_task {
	const char *id;
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
	const char *name;
	
	char stop;
	unsigned char flags;
	
	CRTX_EVENT_TYPE_VARTYPE *types;
	unsigned int n_types;
	
	char **descriptions;
	unsigned int n_descriptions;
	
	pthread_mutex_t mutex;
	
	struct crtx_task *tasks;
	
	enum crtx_processing_mode mode;
	
	struct crtx_dll *equeue;
	
	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_cond;
	
	struct crtx_listener_base *listener;
	
	struct crtx_thread_job_description thread_job;
	
	struct crtx_dll ll_hdr;
};

struct crtx_listener_repository {
	char *id;
	
	struct crtx_listener_base *(*create)(void *options);
};

enum crtx_listener_state { CRTX_LSTNR_UNKNOWN=0, CRTX_LSTNR_STARTING, CRTX_LSTNR_STARTED, CRTX_LSTNR_PAUSED, CRTX_LSTNR_STOPPING, CRTX_LSTNR_STOPPED, CRTX_LSTNR_SHUTDOWN }; //CRTX_LSTNR_ABORTED, 

// immediately schedule a call of the fd event handler (e.g., to query flags/timeouts/...)
#define CRTX_LSTNR_STARTUP_TRIGGER (1<<0)

struct crtx_listener_base {
	struct crtx_ll ll;
	
	const char *id;
	char *name;
	
	MUTEX_TYPE state_mutex;
	enum crtx_listener_state state;
	struct crtx_graph *state_graph;
	
	enum crtx_processing_mode mode;
	
	struct crtx_evloop_fd evloop_fd;
	struct crtx_evloop_callback default_el_cb;
	
	char autolock_source;
	MUTEX_TYPE source_lock;
	
	struct crtx_ll *dependencies;
	struct crtx_ll *rev_dependencies;
	MUTEX_TYPE dependencies_lock;
	
	struct crtx_thread_job_description thread_job;
	struct crtx_thread *eloop_thread;
// 	thread_fct thread_fct;
// 	void *thread_data;
	void (*thread_stop)(struct crtx_thread *thread, void *data);
	
	char (*start_listener)(struct crtx_listener_base *listener);
	char (*stop_listener)(struct crtx_listener_base *listener);
	char (*update_listener)(struct crtx_listener_base *listener);
	
	void (*shutdown)(struct crtx_listener_base *base);
	void (*free_cb)(struct crtx_listener_base *base, void *userdata);
	void *free_cb_userdata;
	
	struct crtx_graph *graph;
	
	unsigned int flags;
	char free_after_event;
};

struct crtx_module {
	char *id;
	
	void (*init)();
	void (*finish)();
};

struct crtx_lstnr_plugin {
	char *path;
	char *basename;
	
	void *handle;
	
	char initialized;
	char (*init)();
	void (*finish)();
	void (*get_listener_repository)(struct crtx_listener_repository **listener_repository, unsigned int *listener_repository_length);
	
	char *plugin_name;
};

struct crtx_handler_category_entry {
	char *handler_name;
	crtx_handle_task_t function;
	void *handler_data;
};

struct crtx_handler_category {
	char *event_type;
	struct crtx_ll *entries;
};

struct crtx_root {
	struct crtx_graph **graphs;
	unsigned int n_graphs;
	MUTEX_TYPE graphs_mutex;
	
	struct crtx_ll *listeners;
	MUTEX_TYPE listeners_mutex;
	
	struct crtx_dll *graph_queue;
	MUTEX_TYPE graph_queue_mutex;
	
// 	char after_fork_close;
	char shutdown;
	char reinit_after_shutdown;
	struct crtx_evloop_callback shutdown_el_cb;
	void (*reinit_cb)(void *cb_data);
	void *reinit_cb_data;
	
	const char *chosen_event_loop;
// 	char detached_event_loop;
	struct crtx_event_loop *event_loop;
	struct crtx_ll *event_loops;
	
	struct crtx_thread_job_description evloop_job;
	struct crtx_thread *evloop_thread;
	
	enum crtx_processing_mode default_mode;
	enum crtx_processing_mode force_mode;
	
	struct crtx_graph *crtx_ctrl_graph;
	
// 	void *notification_listeners_handle;
	
	struct crtx_lstnr_plugin *plugins;
	unsigned int n_plugins;
	
	struct crtx_listener_repository *listener_repository;
	unsigned int listener_repository_length;
	
	struct crtx_ll *handler_categories;
	
	int global_fd_flags;
};

extern struct crtx_module static_modules[];
// extern struct crtx_graph *cortexd_graph;

// extern struct crtx_graph **graphs;
// extern unsigned int n_graphs;

#define CRTX_EVT_SHUTDOWN "cortexd.control.shutdown"
#define CRTX_EVT_MOD_INIT "cortex.control.module_initialized"
// extern struct crtx_graph *crtx_ctrl_graph;
extern struct crtx_root *crtx_root;


void crtx_printf(char level, char const *format, ...) __attribute__ ((format (printf, 2, 3)));
char *crtx_stracpy(const char *str, size_t *str_length);
struct crtx_event *new_event();
void free_event(struct crtx_event *event);
int crtx_init();
int crtx_finish();
void crtx_loop();
int crtx_loop_onetime();

int crtx_create_graph(struct crtx_graph **crtx_graph, const char *name);
int crtx_init_graph(struct crtx_graph *crtx_graph, const char *name);
int crtx_shutdown_graph(struct crtx_graph *egraph);
int crtx_free_graph(struct crtx_graph *egraph);

// void get_eventgraph(struct crtx_graph **crtx_graph, char **event_types, unsigned int n_event_types);
void add_task(struct crtx_graph *graph, struct crtx_task *task);
void add_event_type(char *event_type);
void add_raw_event(struct crtx_event *event);
void crtx_add_event(struct crtx_graph *graph, struct crtx_event *event);
void add_event_sync(struct crtx_graph *graph, struct crtx_event *event);
struct crtx_graph *crtx_find_graph_for_event_description(char *event_description);
struct crtx_graph *crtx_find_graph_for_event_type(CRTX_EVENT_TYPE_VARTYPE event_type);
struct crtx_graph *find_graph_by_name(char *na6me);
struct crtx_graph *crtx_get_graph_for_event_description(char *event_type, char **new_event_types);
struct crtx_task *new_task();
void crtx_free_task(struct crtx_task *task);
char wait_on_event(struct crtx_event *event);
int crtx_create_listener(const char *id, void *options);
int crtx_init_listener_base(struct crtx_listener_base *lstnr);

// void free_listener(struct crtx_listener_base *listener);
void crtx_free_listener(struct crtx_listener_base *listener);
int crtx_create_event(struct crtx_event **event);
struct crtx_task *crtx_create_task(struct crtx_graph *graph, unsigned char position, const char *id, crtx_handle_task_t handler, void *userdata);
// void crtx_claim_next_event(struct crtx_dll **graph, struct crtx_dll **event);
void crtx_process_event(struct crtx_graph *graph, struct crtx_dll *queue_entry);
void crtx_init_shutdown();

int crtx_start_listener(struct crtx_listener_base *listener);
int crtx_update_listener(struct crtx_listener_base *listener);
int crtx_stop_listener(struct crtx_listener_base *listener);
void print_tasks(struct crtx_graph *graph);
void hexdump(unsigned char *buffer, size_t index);

void crtx_daemonize();

void crtx_traverse_graph(struct crtx_graph *graph, struct crtx_event *event);
void reference_event_response(struct crtx_event *event);
void dereference_event_response(struct crtx_event *event);
void reference_event_release(struct crtx_event *event);
void dereference_event_release(struct crtx_event *event);

// void event_ll_add(struct crtx_event_ll **list, struct crtx_event *event);
char *get_username();
char is_graph_empty(struct crtx_graph *graph, char *event_type);
// void *crtx_copy_raw_data(struct crtx_event_data *data);

void crtx_init_notification_listeners(void **data);
void crtx_finish_notification_listeners(void *data);

int crtx_handle_std_signals();

void *crtx_process_graph_tmain(void *arg);

// void crtx_event_set_data(struct crtx_event *event, void *raw_pointer, unsigned char flags, struct crtx_dict *data_dict, unsigned char n_additional_fields);
void crtx_event_set_raw_data(struct crtx_event *event, unsigned char type, ...);
void crtx_event_set_dict_data(struct crtx_event *event, struct crtx_dict *data_dict, unsigned char n_additional_fields);
char crtx_event_raw2dict(struct crtx_event *event, void *user_data);
void crtx_event_get_payload(struct crtx_event *event, char *id, void **raw_pointer, struct crtx_dict **dict);
struct crtx_dict_item *crtx_event_get_item_by_key(struct crtx_event *event, char *id, char *key);
int crtx_event_get_value_by_key(struct crtx_event *event, char *key, char type, void *buffer, size_t buffer_size);
void *crtx_event_get_ptr(struct crtx_event *event);
char *crtx_event_get_string(struct crtx_event *event, char *key);

void crtx_register_handler_for_event_type(char *event_type, char *handler_name, crtx_handle_task_t handler_function, void *handler_data);
void crtx_autofill_graph_with_tasks(struct crtx_graph *graph, const char *event_type);

enum crtx_processing_mode crtx_get_mode(enum crtx_processing_mode local_mode);
void crtx_wait_on_graph_empty(struct crtx_graph *graph);

struct crtx_listener_repository* crtx_get_new_listener_repo_entry();

void crtx_set_main_event_loop(const char *event_loop);

void crtx_lock_listener_source(struct crtx_listener_base *lbase);
void crtx_unlock_listener_source(struct crtx_listener_base *lbase);
void crtx_trigger_event_processing(struct crtx_listener_base *lstnr);

void crtx_shutdown_after_fork();

struct crtx_thread * crtx_start_detached_event_loop();

#endif
