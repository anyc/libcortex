#ifndef _CRTX_CORE_H
#define _CRTX_CORE_H

/** \file core.h
 * The core definitions of libcortex
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdint.h>
#include <regex.h>

#include "config.h"
#include "threads.h"
#include "dict.h"
#include "linkedlist.h"
#include "evloop.h"

#define CRTX_TEST_MAIN(mainfct) \
	int main(int argc, char **argv) { \
		int i; \
		crtx_init(); \
		crtx_handle_std_signals(); \
		CRTX_DBG("entering test function\n"); \
		i = mainfct(argc, argv); \
		CRTX_DBG("left test function\n"); \
		crtx_finish(); \
		return i; \
	}

/// specifies how a listener should be processed
enum crtx_processing_mode {
	CRTX_PREFER_NONE=0,		///< unspecified mode
	CRTX_PREFER_THREAD,		///< a thread should be started, e.g., for blocking calls
	CRTX_PREFER_ELOOP,		///< a listener provides a file descriptor that can be added to the event loop
	CRTX_NO_PROCESSING_MODE	///< the listener requires no processing, e.g., if it depends on other listeners
};

struct crtx_event;
struct crtx_listener_base;
struct crtx_graph;

typedef void (*crtx_raw_to_dict_t)(struct crtx_event *event, struct crtx_dict_item *item, void *user_data);

#ifndef CRTX_EVENT_TYPE_VARTYPE
#define CRTX_EVENT_TYPE_VARTYPE unsigned int
#endif

#define CRTX_EVENT_TYPE_FAMILY_MAX (2)

/// an event that is emitted by a listener \ref crtx_listener_base
struct crtx_event {
	CRTX_EVENT_TYPE_VARTYPE type; ///< this or \ref crtx_event.description describe the type of event
	char *description; ///< this or \ref crtx_event.type describe the type of event
	
	struct crtx_dict_item data; ///< data associated with this event
	
	struct crtx_dict_item response;
	char response_expected;
	
	char error; ///< if set, event has been invalidated
	char release_in_progress;
	
	unsigned char refs_before_response;
	unsigned char refs_before_release;
	pthread_cond_t response_cond;
	pthread_cond_t release_cond;
	
	struct crtx_listener_base *origin; ///< the listener that generated this event
	
	void (*cb_before_release)(struct crtx_event *event, void *cb_before_release_data);
	void *cb_before_release_data;
	
	pthread_mutex_t mutex; ///< mutex that protects this structure
	
	#ifndef CRTX_REDUCED_SIZE
	/* reserved to avoid ABI breakage */
	void *reserved1;
	void *reserved2;
	#endif
};

/// a task basically represents a function that will be executed for an \ref crtx_event
struct crtx_task {
	const char *id;
	unsigned char position; ///< position of the task in the graph
	struct crtx_graph *graph;
	
	char *event_type_match; ///< if 0, task receives all events passing the graph
	
	crtx_handle_task_t handle; ///< function that is called with the current event
	crtx_handle_task_t cleanup;
	void *userdata;
	
	struct crtx_task *prev;
	struct crtx_task *next;
	
	#ifndef CRTX_REDUCED_SIZE
	/* reserved to avoid ABI breakage */
	void *reserved1;
	void *reserved2;
	#endif
};

#define CRTX_GRAPH_KEEP_GOING 1<<0

/// structure that represents a graph of tasks (\ref crtx_task) that will be traversed with every \ref crtx_event
struct crtx_graph {
	const char *name;
	
	char stop;
	unsigned char flags;
	
	CRTX_EVENT_TYPE_VARTYPE *types; ///< type of events this graph accepts
	unsigned int n_types; ///< number of \ref crtx_graph.types
	
	char **descriptions; ///< event descriptions this graph accepts
	unsigned int n_descriptions; ///< number of \ref crtx_graph.descriptions
	
	pthread_mutex_t mutex; ///< mutex that protects this structure
	
	struct crtx_task *tasks; ///< list of tasks in this graph
	
	enum crtx_processing_mode mode; ///< processing mode (none, thread, eloop) of this graph
	
	struct crtx_dll *equeue; ///< event queue of this graph
	struct crtx_dll *claimed; ///< list of events which are currently processed
	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_cond;
	
	struct crtx_listener_base *listener; ///< listener this graph belongs to
	
	struct crtx_thread_job_description thread_job;
	
	struct crtx_dll ll_hdr;
	
	#ifndef CRTX_REDUCED_SIZE
	/* reserved to avoid ABI breakage */
	void *reserved1;
	void *reserved2;
	#endif
};

/// listener repository to create new listeners of a certain type
struct crtx_listener_repository {
	char *id;
	
	struct crtx_listener_base *(*create)(void *options);
	
	#ifndef CRTX_REDUCED_SIZE
	/* reserved to avoid ABI breakage */
	void *reserved1;
	void *reserved2;
	#endif
};

/// the possible states of a listener \ref crtx_listener_base
enum crtx_listener_state {
	CRTX_LSTNR_UNKNOWN=0,
	CRTX_LSTNR_STARTING,
	CRTX_LSTNR_STARTED,
	CRTX_LSTNR_PAUSED,
	CRTX_LSTNR_STOPPING,
	CRTX_LSTNR_STOPPED,
	CRTX_LSTNR_SHUTTING_DOWN,
	CRTX_LSTNR_SHUTDOWN
};

// immediately schedule a call of the fd event handler (e.g., to query flags/timeouts/...)
#define CRTX_LSTNR_STARTUP_TRIGGER	(1<<0)
#define CRTX_LSTNR_NO_AUTO_CLOSE 	(1<<1)

/** base structure of a listener
 *
 * Every libcortex listener "inherits" from this structure by using it as the first
 * member of its structure. This structure contains the basic information needed for
 * libcortex.
 */
struct crtx_listener_base {
	struct crtx_ll ll; ///< linked-list entry of this listener
	
	const char *id;
	char *name;
	
	MUTEX_TYPE state_mutex; ///< mutex that protects the listener's state
	enum crtx_listener_state state; ///< the current state of this listener
	struct crtx_graph *state_graph;
	
	enum crtx_processing_mode mode; ///< processing mode (none, thread, eloop) of this listener
	
	struct crtx_evloop_fd evloop_fd; ///< if this listener has a file descriptor, it will be stored in this structure
	struct crtx_evloop_callback default_el_cb; ///< contains data about what should happen if events occur
	
	char autolock_source; ///< if enabled, \ref crtx_listener_base.source_lock is acquired before calling an event handler
	MUTEX_TYPE source_lock;
	
	struct crtx_ll *dependencies; ///< list of listeners this listener depends on
	struct crtx_ll *rev_dependencies; ///< list of listeners which depend on us
	MUTEX_TYPE dependencies_lock; ///< mutex that protects the (reverse) depenency lists
	
	struct crtx_thread_job_description thread_job;
	struct crtx_thread *eloop_thread;
	void (*thread_stop)(struct crtx_thread *thread, void *data);
	
	char (*start_listener)(struct crtx_listener_base *listener);
	char (*stop_listener)(struct crtx_listener_base *listener);
	char (*update_listener)(struct crtx_listener_base *listener);
	
	void (*shutdown)(struct crtx_listener_base *base);
	void (*free_cb)(struct crtx_listener_base *base, void *userdata);
	void *free_cb_userdata;
	
	struct crtx_graph *graph; ///< graph that contains the tasks that will be executed on events
	
	unsigned int flags;
	char free_after_event;
	
	void *userdata; ///< storage for user-provided data
	
	#ifndef CRTX_REDUCED_SIZE
	/* reserved to avoid ABI breakage */
	void *reserved1;
	void *reserved2;
	#endif
};

/// represents generic modules
struct crtx_module {
	char *id;
	
	void (*init)();
	void (*finish)();
};

/** structure that represents a plugin
 */
struct crtx_lstnr_plugin {
	char *path; ///< path to the library
	char *basename; ///< filename of the library
	
	void *handle; ///< library handle
	
	char initialized;
	char (*init)(); ///< callback that initializes this plugin
	void (*finish)(); ///< callback that finishes this plugin
	/// callback that registers the listeners the plugin provides
	void (*get_listener_repository)(struct crtx_listener_repository **listener_repository, unsigned int *listener_repository_length);
	
	char *plugin_name; ///< name of this plugin
	
	#ifndef CRTX_REDUCED_SIZE
	/* reserved to avoid ABI breakage */
	void *reserved1;
	void *reserved2;
	#endif
};

struct crtx_handler_category_entry {
	char *handler_name;
	crtx_handle_task_t function;
	void *handler_data;
};

/// a list of \ref crtx_handler_category_entry for a certain event type
struct crtx_handler_category {
	char *event_type;
	struct crtx_ll *entries;
};

/** core structure of libcortex
 */
struct crtx_root {
	struct crtx_graph **graphs;
	unsigned int n_graphs;
	MUTEX_TYPE graphs_mutex;
	
	struct crtx_ll *listeners; ///< list of registered listeners
	MUTEX_TYPE listeners_mutex; ///< mutex that protects the list of listeners \ref crtx_root.listeners
	
	struct crtx_dll *graph_queue;
	MUTEX_TYPE graph_queue_mutex;
	
	char initialized;
	char shutdown;
	char reinit_after_shutdown; ///< if true, reinitialize libcortex after a fork
	struct crtx_evloop_callback shutdown_el_cb;
	void (*reinit_cb)(void *cb_data); ///< callback function that is called during reinit after a fork
	void *reinit_cb_data;
	
	const char *chosen_event_loop;
	struct crtx_event_loop *event_loop;
	struct crtx_ll *event_loops;
	
	struct crtx_thread_job_description evloop_job;
	struct crtx_thread *evloop_thread;
	
	enum crtx_processing_mode default_mode;
	enum crtx_processing_mode force_mode;
	
	struct crtx_graph *crtx_ctrl_graph;
	
	struct crtx_lstnr_plugin *plugins; ///< list of available plugins
	unsigned int n_plugins; ///< number of available plugins in \ref crtx_root.plugins
	
	struct crtx_listener_repository *listener_repository;
	unsigned int listener_repository_length;
	
	struct crtx_ll *handler_categories;
	
	int global_fd_flags;
	
	struct crtx_listener_base *selfpipe_lstnr;
	
	MUTEX_TYPE mutex;
	
	#ifndef CRTX_REDUCED_SIZE
	/* reserved to avoid ABI breakage */
	void *reserved1;
	void *reserved2;
	#endif
};

extern struct crtx_module static_modules[];
extern struct crtx_root *crtx_root;


void crtx_printf(char level, char const *format, ...) __attribute__ ((format (printf, 2, 3)));
char *crtx_stracpy(const char *str, size_t *str_length);
struct crtx_event *new_event();
void crtx_free_event(struct crtx_event *event);
int crtx_init();
int crtx_finish();
void crtx_loop();
int crtx_loop_onetime();
int crtx_is_shutting_down();

int crtx_create_graph(struct crtx_graph **crtx_graph, const char *name);
int crtx_init_graph(struct crtx_graph *crtx_graph, const char *name);
int crtx_shutdown_graph(struct crtx_graph *egraph);
int crtx_free_graph(struct crtx_graph *egraph);

// void get_eventgraph(struct crtx_graph **crtx_graph, char **event_types, unsigned int n_event_types);
void add_task(struct crtx_graph *graph, struct crtx_task *task);
void crtx_add_task(struct crtx_graph *graph, struct crtx_task *task);
struct crtx_task * crtx_graph_get_task(struct crtx_graph *graph, const char *id, crtx_handle_task_t handler);
void add_event_type(char *event_type);
int crtx_find_graph_add_event(struct crtx_event *event);
void crtx_add_event(struct crtx_graph *graph, struct crtx_event *event);
void add_event_sync(struct crtx_graph *graph, struct crtx_event *event);
struct crtx_graph *crtx_find_graph_for_event_description(char *event_description);
struct crtx_graph *crtx_find_graph_for_event_type(CRTX_EVENT_TYPE_VARTYPE event_type);
struct crtx_graph *crtx_get_graph_for_event_description(char *event_type, char **new_event_types);
struct crtx_task *crtx_new_task();
void crtx_free_task(struct crtx_task *task);
int crtx_wait_on_event(struct crtx_event *event);
int crtx_create_listener(const char *id, void *options) __attribute__((deprecated));
int crtx_setup_listener(const char *id, void *options);
int crtx_init_listener_base(struct crtx_listener_base *lstnr);
struct crtx_listener_base *crtx_calloc_listener_base(void);
int crtx_listener_add_fd(struct crtx_listener_base *listener,
						 int *fd_ptr,
						 int fd,
						 int event_flags,
						 uint64_t timeout_us,
						 crtx_handle_task_t event_handler,
						 void *event_handler_data,
						 crtx_evloop_error_cb_t error_cb,
						 void *error_cb_data
		);
int crtx_listener_get_fd(struct crtx_listener_base *listener);

void crtx_shutdown_listener(struct crtx_listener_base *listener);
int crtx_create_event(struct crtx_event **event);
int crtx_push_new_event(struct crtx_listener_base *lstnr, struct crtx_event **event,
						CRTX_EVENT_TYPE_VARTYPE event_type, char *description,
						char data_type, char *data_key_or_sign,
						...);
struct crtx_task *crtx_create_task(struct crtx_graph *graph, unsigned char position, const char *id, crtx_handle_task_t handler, void *userdata);
struct crtx_task *crtx_create_task_unique(struct crtx_graph *graph, unsigned char position, const char *id, crtx_handle_task_t handler, void *userdata);
void crtx_init_shutdown();

int crtx_start_listener(struct crtx_listener_base *listener);
int crtx_update_listener(struct crtx_listener_base *listener);
int crtx_stop_listener(struct crtx_listener_base *listener);
void crtx_print_tasks(struct crtx_graph *graph);
void crtx_hexdump(unsigned char *buffer, size_t index);

void crtx_traverse_graph(struct crtx_graph *graph, struct crtx_event *event);
void crtx_reference_event_response(struct crtx_event *event);
void crtx_dereference_event_response(struct crtx_event *event);
void crtx_reference_event_release(struct crtx_event *event);
void crtx_dereference_event_release(struct crtx_event *event);

// void event_ll_add(struct crtx_event_ll **list, struct crtx_event *event);
int crtx_graph_has_task(struct crtx_graph *graph);
int crtx_is_graph_empty(struct crtx_graph *graph);
// void *crtx_copy_raw_data(struct crtx_event_data *data);

void crtx_init_notification_listeners(void **data);
void crtx_finish_notification_listeners(void *data);

int crtx_handle_std_signals();

void *crtx_process_graph_tmain(void *arg);
int crtx_process_one_event(struct crtx_graph *graph);

// void crtx_event_set_data(struct crtx_event *event, void *raw_pointer, unsigned char flags, struct crtx_dict *data_dict, unsigned char n_additional_fields);
void crtx_event_set_raw_data(struct crtx_event *event, unsigned char type, ...);
void crtx_event_set_dict_data(struct crtx_event *event, struct crtx_dict *data_dict, unsigned char n_additional_fields);
int crtx_event_raw2dict(struct crtx_event *event, void *user_data);
void crtx_event_get_payload(struct crtx_event *event, char *id, void **raw_pointer, struct crtx_dict **dict);
struct crtx_dict *crtx_event_get_dict(struct crtx_event *event);
struct crtx_dict_item *crtx_event_get_item_by_key(struct crtx_event *event, char *id, char *key);
int crtx_event_get_value_by_key(struct crtx_event *event, char *key, char type, void *buffer, size_t buffer_size);
void *crtx_event_get_ptr(struct crtx_event *event);
char *crtx_event_get_string(struct crtx_event *event, char *key);
void crtx_event_set_dict(struct crtx_event *event, char *signature, ...);
int crtx_event_get_int(struct crtx_event *event, int *value);
int crtx_event_get_data(struct crtx_event *event, void **data, size_t *size);

void crtx_register_handler_for_event_type(char *event_type, char *handler_name, crtx_handle_task_t handler_function, void *handler_data);
void crtx_autofill_graph_with_tasks(struct crtx_graph *graph, const char *event_type);

enum crtx_processing_mode crtx_get_mode(enum crtx_processing_mode local_mode);
void crtx_wait_on_graph_empty(struct crtx_graph *graph);

struct crtx_listener_repository* crtx_get_new_listener_repo_entry();

void crtx_set_main_event_loop(const char *event_loop);
int crtx_separate_evloop_thread();

void crtx_lock_listener_source(struct crtx_listener_base *lbase);
void crtx_unlock_listener_source(struct crtx_listener_base *lbase);
void crtx_lock_listener(struct crtx_listener_base *lbase);
void crtx_unlock_listener(struct crtx_listener_base *lbase);
void crtx_trigger_event_processing(struct crtx_listener_base *lstnr);
void crtx_lstnr_handle_fd_closed(struct crtx_listener_base *lstnr);

void crtx_shutdown_after_fork();

struct crtx_thread * crtx_start_detached_event_loop();

int crtx_get_version(unsigned int *major, unsigned int *minor, unsigned int *revision);

void crtx_selfpipe_cb_free_lstnr(void *data);
int crtx_selfpipe_enqueue_cb(void (*cb)(void*), void *cb_data);

void crtx_print_event(struct crtx_event *event, FILE *f);

int crtx_is_fd_valid(int fd);

#ifdef __cplusplus
}
#endif

#endif
