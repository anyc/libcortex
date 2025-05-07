/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <regex.h>
#include <stddef.h>
#include <inttypes.h>
#include <errno.h>
#include <syslog.h>

// for get_username()
#include <unistd.h>
#include <pwd.h>

#include <fcntl.h>
#include <sys/stat.h>

#include "intern.h"
#include "core.h"
#include "dict.h"
#include "pipe.h"


#ifndef CRTX_PLUGIN_DIR
#define CRTX_PLUGIN_DIR "/usr/lib/"
#endif

struct crtx_root crtx_global_root = { 0 };
struct crtx_root *crtx_root = &crtx_global_root;

#include "core_modules.h"

char crtx_verbosity = CRTX_VLEVEL_INFO;
char crtx_pid_prefix = 0;
char crtx_use_syslog = 0;

char *crtx_stracpy(const char *str, size_t *str_length) {
	char *r;
	size_t length;
	
	if (!str) {
		CRTX_INFO("trying to stracpy null pointer\n");
		if (str_length)
			*str_length = 0;
		
		return 0;
	}
	
	if (!str_length || *str_length == 0) {
		length = strlen(str);
		if (str_length && *str_length == 0)
			*str_length = length;
	} else {
		length = *str_length;
	}
	
	r = (char*) malloc(length+1);
	if (!r)
		return 0;
	snprintf(r, length+1, "%s", str);
	
	return r;
}

void crtx_printf_va(char level, char const *format, va_list va) {
	if (level == CRTX_VLEVEL_ERR) {
		if (crtx_use_syslog) {
			syslog(LOG_ERR, "CRTX: ");
			vsyslog(LOG_ERR, format, va);
		} else {
			if (crtx_pid_prefix)
				fprintf(stderr, "[%d] ", getpid());
			vfprintf(stderr, format, va);
		}
	} else {
		if (level <= crtx_verbosity) {
			if (crtx_use_syslog) {
				syslog(LOG_NOTICE, "CRTX: ");
				vsyslog(LOG_NOTICE, format, va);
			} else {
				if (crtx_pid_prefix)
					fprintf(stdout, "[%d] ", getpid());
				vfprintf(stdout, format, va);
				fflush(stdout);
			}
		}
	}
}

void crtx_printf(char level, char const *format, ...) {
	va_list va;
	
	va_start(va, format);
	
	crtx_printf_va(level, format, va);
	
	va_end(va);
}

enum crtx_processing_mode crtx_get_mode(enum crtx_processing_mode local_mode) {
	enum crtx_processing_mode mode;
	
	mode = local_mode;
	if (crtx_root->force_mode == CRTX_PREFER_NONE) {
		if (local_mode == CRTX_PREFER_NONE) {
			mode = crtx_root->default_mode;
		}
	} else
	if (crtx_root->force_mode == CRTX_PREFER_THREAD) {
		mode = CRTX_PREFER_THREAD;
	} else
	if (crtx_root->force_mode == CRTX_PREFER_ELOOP) {
		mode = CRTX_PREFER_ELOOP;
	}
	
	return mode;
}

static void crtx_default_el_error_cb(struct crtx_evloop_callback *el_cb, void *data) {
	struct crtx_listener_base *listener;
	
	listener = (struct crtx_listener_base *) data;
	
	crtx_shutdown_listener(listener);
}

/// start or activate a listener
int crtx_start_listener(struct crtx_listener_base *listener) {
	enum crtx_processing_mode mode;
	struct crtx_event *event;
	int ret;
	
	LOCK(listener->state_mutex);
	
	if (listener->state == CRTX_LSTNR_STARTED) {
		CRTX_DBG("listener already started\n");
		UNLOCK(listener->state_mutex);
		return -EINPROGRESS;
	}
	
	listener->state = CRTX_LSTNR_STARTING;
	
	CRTX_DBG("starting listener \"%s\"", listener->id);
	if (listener->evloop_fd.fd >=0)
		CRTX_DBG(" (fd %d)", *listener->evloop_fd.fd);
	CRTX_DBG("\n");
	
	// execute a start callback function which could abort the startup
	if (listener->start_listener) {
		ret = listener->start_listener(listener);
		if (ret < 0) {
			CRTX_ERROR("start_listener \"%s\" failed\n", listener->id);
			
			listener->state = CRTX_LSTNR_STOPPED;
			
			if (listener->state_graph) {
				ret = crtx_create_event(&event);
				if (ret) {
					CRTX_ERROR("crtx_create_event failed: %s\n", strerror(ret));
				} else {
					event->description = "listener_state";
					event->data.type = 'u';
					event->data.uint32 = CRTX_LSTNR_STOPPED;
					
					crtx_add_event(listener->state_graph, event);
				}
			}
			
			UNLOCK(listener->state_mutex);
			
			return ret;
		}
	}
	
	// start_listener() might modify listener->mode
	mode = crtx_get_mode(listener->mode);
	
	if (listener->evloop_fd.fd < 0 && !listener->thread_job.fct && !listener->start_listener && mode != CRTX_NO_PROCESSING_MODE) {
		CRTX_DBG("no method to start listener \"%s\" provided (%d, %p, %p, %d)\n", listener->id, *listener->evloop_fd.fd, listener->thread_job.fct, listener->start_listener, listener->mode);
		
		UNLOCK(listener->state_mutex);
		return -EINVAL;
	}
	
	// actually activate the listener by adding a fd to the evloop or starting a thread
	if (listener->evloop_fd.fd || listener->thread_job.fct) {
		if (mode == CRTX_PREFER_ELOOP && listener->evloop_fd.fd <= 0) {
			CRTX_ERROR("listener mode set to \"event loop\" but no event loop data available (fd = %d)\n", *listener->evloop_fd.fd);
			mode = CRTX_PREFER_THREAD;
		}
		if (mode == CRTX_PREFER_THREAD && !listener->thread_job.fct) {
			CRTX_ERROR("listener mode set to \"thread\" but no thread data available\n");
			if (listener->evloop_fd.fd > 0)
				mode = CRTX_PREFER_ELOOP;
			else
				mode = 0;
		}
		
		if (mode == CRTX_PREFER_THREAD) {
			listener->eloop_thread = crtx_thread_assign_job(&listener->thread_job);
			
			crtx_reference_signal(&listener->eloop_thread->finished);
			
			crtx_thread_start_job(listener->eloop_thread);
		} else 
		if (mode == CRTX_PREFER_ELOOP) {
			if (listener->evloop_fd.callbacks->error_cb == 0) {
				listener->evloop_fd.callbacks->error_cb = crtx_default_el_error_cb;
				listener->evloop_fd.callbacks->error_cb_data = listener;
			}
			
			crtx_evloop_enable_cb(&listener->default_el_cb);
		} else
		if (mode == CRTX_NO_PROCESSING_MODE) {
			// do nothing
		} else {
			CRTX_ERROR("invalid start listener mode: %d\n", mode);
			UNLOCK(listener->state_mutex);
			return -EINVAL;
		}
	}
	listener->state = CRTX_LSTNR_STARTED;
	
	// create a start event if requested
	if (listener->state_graph) {
		ret = crtx_create_event(&event);
		if (ret) {
			CRTX_ERROR("crtx_create_event failed: %s\n", strerror(ret));
		} else {
			event->description = "listener_state";
			event->data.type = 'u';
			event->data.uint32 = CRTX_LSTNR_STARTED;
			crtx_add_event(listener->state_graph, event);
		}
	}
	
	// immediately queue this listener so our event handler will be called
	if (listener->flags & CRTX_LSTNR_STARTUP_TRIGGER)
		crtx_trigger_event_processing(listener);
	
	UNLOCK(listener->state_mutex);
	
	return 0;
}

/// trigger a listener update, e.g., to apply changes to a running listener
int crtx_update_listener(struct crtx_listener_base *listener) {
	if (listener->state != CRTX_LSTNR_STARTED) {
		return crtx_start_listener(listener);
	}
	
	if (listener->update_listener) {
		return listener->update_listener(listener);
	}
	
	return 0;
}

struct crtx_listener_base *crtx_calloc_listener_base(void) {
	return (struct crtx_listener_base *) calloc(1, sizeof(struct crtx_listener_base));
}

int crtx_init_listener_base(struct crtx_listener_base *lbase) {
	INIT_MUTEX(lbase->state_mutex);
	
	lbase->state = CRTX_LSTNR_STOPPED;
	
	if (!lbase->graph) {
		crtx_create_graph(&lbase->graph, lbase->id);
		lbase->graph->listener = lbase;
	}
	
	INIT_REC_MUTEX(lbase->source_lock);
	
	if (!lbase->evloop_fd.fd) {
		lbase->evloop_fd.fd = &lbase->evloop_fd.l_fd;
		lbase->evloop_fd.l_fd = -1;
	}
	
	return 0;
}

/// look for a listener repo for the given id and iniitalize a listener of this type
int crtx_setup_listener(const char *id, void *options) {
	struct crtx_listener_repository *l;
	struct crtx_listener_base *lbase;
	char static_lstnr;
	
	
	if (id == 0)
		return -EINVAL;
	
	if (id[0] != 0) {
		CRTX_DBG("new listener %s\n", id);
		
		static_lstnr = 1;
		lbase = 0;
		l = static_listener_repository;
		while (l && l->id) {
			while (l->id) {
				if (!strcmp(l->id, id)) {
					crtx_init_listener_base( (struct crtx_listener_base *) options );
					
					lbase = l->create(options);
					
					if (!lbase) {
						CRTX_ERROR("creating listener \"%s\" failed: create procedure failed\n", id);
						return 1; // TODO get error code from create()
					}
					
					break;
				}
				l++;
			}
			if (lbase)
				break;
			
			if (static_lstnr) {
				l = crtx_root->listener_repository;
				static_lstnr = 0;
			}
		}
		
		if (!lbase) {
			CRTX_ERROR("creating listener \"%s\" failed: not found\n", id);
			return -ENOENT;
		}
	} else {
		lbase = (struct crtx_listener_base *) options;
		
		crtx_init_listener_base(lbase);
	}
	
	lbase->id = id;
	
	LOCK(crtx_root->listeners_mutex);
	lbase->ll.data = lbase;
	crtx_ll_append(&crtx_root->listeners, &lbase->ll);
	UNLOCK(crtx_root->listeners_mutex);
	
	return 0;
}

int crtx_create_listener(const char *id, void *options) {
	return crtx_setup_listener(id, options);
}

/// deactivate a listener
int crtx_stop_listener(struct crtx_listener_base *listener) {
	LOCK(listener->state_mutex);
	
	if (CRTX_LSTNR_STOPPED <= listener->state && listener->state <= CRTX_LSTNR_SHUTDOWN) {
		UNLOCK(listener->state_mutex);
		return EALREADY;
	}
	
	listener->state = CRTX_LSTNR_STOPPING;
	
	CRTX_DBG("stopping listener %s\n", listener->id);
	
	/// refuse stop if our reverse dependencies are still running
	LOCK(listener->dependencies_lock);
	if (listener->rev_dependencies) {
		struct crtx_ll *ll;
		struct crtx_listener_base *dep;
		int ready;
		
		ready = 1;
		for (ll = listener->rev_dependencies; ll; ll = ll->next) {
			dep = (struct crtx_listener_base *) ll->data;
			if ( dep->state < CRTX_LSTNR_STOPPED || CRTX_LSTNR_SHUTDOWN < dep->state) {
				ready = 0;
				break;
			}
		}
		
		if (!ready) {
			UNLOCK(listener->state_mutex);
			UNLOCK(listener->dependencies_lock);
			
			CRTX_DBG("delayed stop of lstnr %s due to %s\n", listener->id, dep->id);
			
			return 1;
		}
	}
	
	// deregister ourselves at our dependencies
	if (listener->dependencies) {
		struct crtx_ll *ll, *lln, *dll;
		struct crtx_listener_base *dep;
		
		for (ll = listener->dependencies; ll; ll = lln) {
			lln = ll->next;
			
			dep = (struct crtx_listener_base *) ll->data;
			
			for (dll = dep->rev_dependencies; dll; dll=dll->next) {
				if (dll->data == listener) {
					crtx_ll_unlink(&dep->rev_dependencies, dll);
					free(dll);
					break;
				}
			}
			
			crtx_ll_unlink(&listener->dependencies, ll);
			free(ll);
		}
	}
	UNLOCK(listener->dependencies_lock);
	
	// remove us from the event loop
	crtx_evloop_disable_cb(&listener->default_el_cb);
	
	if (listener->eloop_thread) {
		crtx_threads_stop(listener->eloop_thread);
	}
	
	if (listener->stop_listener) {
		listener->stop_listener(listener);
	}
	
	if (listener->eloop_thread) {
		crtx_wait_on_signal(&listener->eloop_thread->finished, 0);
		crtx_dereference_signal(&listener->eloop_thread->finished);
		listener->eloop_thread = 0;
	}
	
	listener->state = CRTX_LSTNR_STOPPED;
	
	// schedule a stop event if requested
	if (listener->state_graph) {
		int ret;
		struct crtx_event *event;
		
		ret = crtx_create_event(&event);
		if (ret < 0) {
			CRTX_ERROR("crtx_create_event failed: %s\n", strerror(ret));
		} else {
			event->description = "listener_state";
			event->data.type = 'u';
			event->data.uint32 = CRTX_LSTNR_STOPPED;
			crtx_add_event(listener->state_graph, event);
		}
	}
	
	UNLOCK(listener->state_mutex);
	
	return 0;
}

// if the listener shutdown is delayed due to pending events, this function will
// be called after the last event was processed
static void shutdown_listener_intern(struct crtx_listener_base *listener) {
	if (listener->graph && listener->graph->listener == listener) {
		crtx_free_graph(listener->graph);
		listener->graph = 0;
	}
	
	if (listener->free_cb) {
		listener->free_cb(listener, listener->free_cb_userdata);
	}
}

/// release this listener
void crtx_shutdown_listener(struct crtx_listener_base *listener) {
	int r;
	
	
	crtx_stop_listener(listener);
	
	LOCK(listener->state_mutex);
	
	if (listener->state < CRTX_LSTNR_STOPPED) {
		CRTX_DBG("listener is not stopped\n");
		UNLOCK(listener->state_mutex);
		return;
	}
	
	if (listener->state == CRTX_LSTNR_SHUTTING_DOWN) {
		UNLOCK(listener->state_mutex);
		return;
	}
	
	if (listener->state == CRTX_LSTNR_SHUTDOWN) {
		if (listener->graph) {
			LOCK(listener->graph->queue_mutex);
			if (!listener->graph->equeue) {
				shutdown_listener_intern(listener);
				return;
			}
			UNLOCK(listener->graph->queue_mutex);
		}
		
		UNLOCK(listener->state_mutex);
		return;
	}
	
	CRTX_DBG("shutdown listener %s (%p)\n", listener->id, listener);
	
	listener->state = CRTX_LSTNR_SHUTTING_DOWN;
	
	UNLOCK(listener->state_mutex);
	
	if (*listener->evloop_fd.fd >= 0) {
		struct crtx_evloop_callback *cit, *citn;
		
		for (cit = listener->evloop_fd.callbacks; cit; cit=citn) {
			citn = (struct crtx_evloop_callback *) cit->ll.next;
			
			crtx_evloop_remove_cb(cit);
			crtx_ll_unlink((struct crtx_ll**) &listener->evloop_fd.callbacks, &cit->ll);
		}
	}
	
	if (listener->shutdown) {
		listener->shutdown(listener);
	} else {
		if (*listener->evloop_fd.fd >= 0 && (listener->flags & CRTX_LSTNR_NO_AUTO_CLOSE) == 0) {
			CRTX_DBG("auto-closing %d during shutdown %s\n", *listener->evloop_fd.fd, listener->id);
			
			close(*listener->evloop_fd.fd);
			*listener->evloop_fd.fd = -1;
		}
	}
	
	LOCK(crtx_root->listeners_mutex);
	r = crtx_ll_unlink(&crtx_root->listeners, &listener->ll);
	if (r) {
		CRTX_ERROR("error unlinking listener %p in global list: %s\n", listener, strerror(-r));
	}
	UNLOCK(crtx_root->listeners_mutex);
	
	/*
	 * TODO the event loop might be shutdown already
	 * Should we process remaining events? Then the event loop should be the last one.
	 * If not - if the shutdown signal is received, we do not care about remaining events
	 */
	
	// the graph can already be freed here, e.g., if the graph is shared with
	// another listener
	if (listener->graph) {
		LOCK(listener->graph->queue_mutex);
		if (listener->graph->equeue) {
			CRTX_DBG("delaying listener shutdown due to pending events\n");
			UNLOCK(listener->graph->queue_mutex);
			
			listener->state = CRTX_LSTNR_SHUTDOWN;
			
			UNLOCK(listener->state_mutex);
			
			return;
		}
		UNLOCK(listener->graph->queue_mutex);
	}
	
	shutdown_listener_intern(listener);
}

/// walk through a graph with one event
static void traverse_graph_r(struct crtx_graph *graph, struct crtx_task *ti, struct crtx_event *event) {
	void *sessiondata[3];
	char keep_going=1;
	
	if (ti->handle && (!ti->event_type_match || !strcmp(ti->event_type_match, event->description))) {
		CRTX_DBG("execute task %s with event %d \"%s\" (%p)\n", ti->id, event->type, event->description, event);
		
		keep_going = ti->handle(event, ti->userdata, sessiondata);
	}
	
	if (ti->next && (
			(!event->response.pointer && !event->response.dict) ||
			keep_going ||
			(graph->flags & CRTX_GRAPH_KEEP_GOING))
		)
	{
		traverse_graph_r(graph, ti->next, event);
	}
	
	if (ti->cleanup) {
		CRTX_DBG("execute task %s with event %d \"%s\" (%p) cleanup\n", ti->id, event->type, event->description, event);
		
		ti->cleanup(event, ti->userdata, sessiondata);
	}
}

void crtx_traverse_graph(struct crtx_graph *graph, struct crtx_event *event) {
	traverse_graph_r(graph, graph->tasks, event);
}

void crtx_claim_next_event_of_graph(struct crtx_graph *graph, struct crtx_dll **event) {
	struct crtx_dll *eit;
	
	if (event && *event)
		eit = *event;
	else
		eit = graph->equeue;
	
	LOCK(graph->queue_mutex);
	for (; eit && (eit->event->claimed || eit->event->error); eit = eit->next) {}
	if (eit && !eit->event->claimed && !eit->event->error) {
		eit->event->claimed = 1;
	}
	UNLOCK(graph->queue_mutex);
	
	if (event)
		*event = eit;
}

int crtx_process_one_event(struct crtx_graph *graph) {
	struct crtx_dll *queue_entry;
	
	
	queue_entry = 0;
	crtx_claim_next_event_of_graph(graph, &queue_entry);
	if (!queue_entry)
		return -EAGAIN;
	
	if (graph->tasks) {
		traverse_graph_r(graph, graph->tasks, queue_entry->event);
	} else {
		if (graph->descriptions && graph->n_descriptions > 0)
			CRTX_INFO("no task in graph type[0] = %s\n", graph->descriptions[0]);
		else
			if (graph->types && graph->types > 0)	
				CRTX_INFO("no task in graph %d\n", graph->types[0]);
		else
			CRTX_INFO("no task in graph\n");
	}
	
	crtx_dereference_event_response(queue_entry->event);
	
	crtx_dereference_event_release(queue_entry->event);
	
	LOCK(graph->queue_mutex);
	if (queue_entry->prev)
		queue_entry->prev->next = queue_entry->next;
	else
		graph->equeue = queue_entry->next;
	if (queue_entry->next)
		queue_entry->next->prev = queue_entry->prev;
	
	if (!graph->equeue)
		pthread_cond_signal(&graph->queue_cond);
	
	free(queue_entry);
	
	if (graph->listener) {
		LOCK(graph->listener->state_mutex);
		if (!graph->equeue && graph->listener->state == CRTX_LSTNR_SHUTDOWN) {
			shutdown_listener_intern(graph->listener);
		} else {
			UNLOCK(graph->listener->state_mutex);
		}
	}
	
	UNLOCK(graph->queue_mutex);
	
	if (graph->listener && graph->listener->free_after_event) {
		crtx_shutdown_listener(graph->listener);
	}
	
	return 1;
}

/// main function of a thread that will process events
static void *process_graph_tmain(void *arg) {
	struct crtx_graph *graph = (struct crtx_graph*) arg;
	int rv;
	
	while (1) {
		rv = crtx_process_one_event(graph);
		// TODO no busy waiting
		if (rv < 0 && rv != -EAGAIN)
			break;
	}
	
	return 0;
}

/// add task to graph
void crtx_add_task(struct crtx_graph *graph, struct crtx_task *task) {
	struct crtx_task *ti, *previous;
	
	task->graph = graph;
	
	if (!graph->tasks) {
		graph->tasks = task;
		task->prev = 0;
		task->next = 0;
		return;
	}
	
	previous = 0;
	for (ti=graph->tasks;ti;ti=ti->next) {
		if (task->position < ti->position) {
			if (previous) {
				task->next = previous->next;
				previous->next = task;
				task->prev = previous;
				task->next->prev = task;
			} else {
				task->next = graph->tasks;
				task->prev = 0;
				graph->tasks = task;
			}
			
			break;
		}
		last = ti;
	}
	if (!ti) {
		task->next = last->next;
		last->next = task;
		task->prev = last;
	}
}

struct crtx_task *crtx_create_task(struct crtx_graph *graph, unsigned char position, const char *id, crtx_handle_task_t handler, void *userdata) {
	struct crtx_task * task;
	
	task = new_task();
	task->id = id;
	task->handle = handler;
	task->userdata = userdata;
	if (position > 0)
		task->position = position;
	crtx_add_task(graph, task);
	
	return task;
}

struct crtx_task * crtx_graph_get_task(struct crtx_graph *graph, const char *id, crtx_handle_task_t handler) {
	struct crtx_task *titer;
	
	for (titer=graph->tasks; titer; titer=titer->next) {
		if (handler && titer->handle == handler)
			break;
		if (id && !strcmp(id, titer->id))
			break;
	}
	
	return titer;
}

int crtx_graph_has_task(struct crtx_graph *graph) {
	return (graph->tasks != 0);
}

// callback function to stop an event processing thread
void graph_consumer_stop(struct crtx_thread *t, void *data) {
	struct crtx_graph *graph = (struct crtx_graph *) data;
	
	graph->stop = 1;
	pthread_cond_broadcast(&graph->queue_cond);
}

int crtx_is_graph_empty(struct crtx_graph *graph) {
	return (graph->equeue == 0);
}

void crtx_wait_on_graph_empty(struct crtx_graph *graph) {
	LOCK(graph->queue_mutex);
	
	while (graph->equeue)
		pthread_cond_wait(&graph->queue_cond, &graph->queue_mutex);
	
	UNLOCK(graph->queue_mutex);
}

/// add a new event to a graph for processing
void crtx_add_event(struct crtx_graph *graph, struct crtx_event *event) {
	if (event->origin) {
		CRTX_ERROR("event %p already has associated listener: %s %p\n", event, event->origin->name, event->origin);
		return;
	}
	
	crtx_reference_event_release(event);
	crtx_reference_event_response(event);
	
	pthread_mutex_lock(&graph->mutex);
	
	if (!graph->tasks) {
		CRTX_INFO("dropping event %s (%p) as graph \"%s\" (%p) is empty\n", event->description, event, graph->name?graph->name:"", graph);
		
		crtx_dereference_event_response(event);
		crtx_dereference_event_release(event);
		
		pthread_mutex_unlock(&graph->mutex);
		return;
	}
	
	CRTX_DBG("adding event %s (%p) to graph \"%s\" (%p)\n", event->description, event, graph->name?graph->name:"", graph);
	
	LOCK(graph->queue_mutex);
	
	event->origin = graph->listener;
	
	crtx_dll_append_new(&graph->equeue, event);
	
	pthread_cond_signal(&graph->queue_cond);
	
	UNLOCK(graph->queue_mutex);
	
	enum crtx_processing_mode mode = crtx_get_mode(graph->mode);
	
	// trigger the processing of the graph
	if (mode == CRTX_PREFER_THREAD) {
		struct crtx_thread *t;
		
		graph->thread_job.fct = &process_graph_tmain;
		graph->thread_job.fct_data = graph;
		graph->thread_job.do_stop = &graph_consumer_stop;
		t = crtx_thread_assign_job(&graph->thread_job);
		crtx_thread_start_job(t);
	} else {
		crtx_evloop_queue_graph(crtx_root->event_loop, graph);
	}
	
	pthread_mutex_unlock(&graph->mutex);
}

int crtx_create_event(struct crtx_event **event) {
	int ret;
	
	if (!event)
		return -EINVAL;
	
	*event = (struct crtx_event*) calloc(1, sizeof(struct crtx_event));
	
	if (!*event)
		return -ENOMEM;
	
	ret = pthread_mutex_init(&(*event)->mutex, 0); CRTX_ASSERT(ret >= 0);
	ret = pthread_cond_init(&(*event)->response_cond, NULL); CRTX_ASSERT(ret >= 0);
	ret = pthread_cond_init(&(*event)->release_cond, NULL); CRTX_ASSERT(ret >= 0);
	
	return 0;
}

/// helper function to create and add an event with one call
int crtx_push_new_event(struct crtx_listener_base *lstnr, struct crtx_event **event,
						CRTX_EVENT_TYPE_VARTYPE event_type, char *event_description,
						char data_type, char *data_key_or_sign,
						...
					   )
{
	va_list va;
	char rv;
	struct crtx_event *levent;
	struct crtx_dict *dict;
	
	
	rv = crtx_create_event(&levent);
	if (rv) {
		CRTX_ERROR("crtx_create_event failed: %s\n", strerror(rv));
		return rv;
	}

	levent->type = event_type;
	levent->description = event_description;
	
	if (data_type) {
		va_start(va, data_key_or_sign);
		
		if (data_type == 'D') {
			dict = crtx_create_dict_va(data_key_or_sign, &va);
			if (!dict) {
				CRTX_ERROR("crtx_create_dict_va() failed: %d\n", rv);
				crtx_free_event(levent);
				return rv;
			}
			
			rv = crtx_fill_data_item(&levent->data, data_type, 0, dict, 0, 0);
			if (rv) {
				CRTX_ERROR("crtx_fill_data_item() failed: %d\n", rv);
				crtx_free_event(levent);
				return rv;
			}
		} else {
			rv = crtx_fill_data_item_va2(&levent->data, data_type, data_key_or_sign, &va);
			if (rv) {
				CRTX_ERROR("crtx_fill_data_item_va2() failed: %d\n", rv);
				crtx_free_event(levent);
				return rv;
			}
		}
		
		va_end(va);
	}
	
	crtx_add_event(lstnr->graph, levent);
	
	if (event)
		*event = levent;
	
	return 0;
}

void crtx_reference_event_release(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	event->refs_before_release++;
	CRTX_VDBG("ref release of event %s (%p) (now %d)\n", event->description, event, event->refs_before_release);
	
	pthread_mutex_unlock(&event->mutex);
}

void crtx_dereference_event_release(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	CRTX_VDBG("deref release of event %s (%p) (remaining %d)\n", event->description, event, event->refs_before_release);
	
	if (event->refs_before_release > 0)
		event->refs_before_release--;
	
	if (event->refs_before_release == 0) {
		pthread_mutex_unlock(&event->mutex);
		crtx_free_event(event);
		return;
	}
	
	pthread_mutex_unlock(&event->mutex);
}

void crtx_reference_event_response(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	if (!event->error) {
		event->refs_before_response++;
		CRTX_VDBG("ref response of event %s (%p) (now %d)\n", event->description, event, event->refs_before_response);
	}
	
	pthread_mutex_unlock(&event->mutex);
}

void crtx_dereference_event_response(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	CRTX_VDBG("ref response of event %s (%p) (remaining %d)\n", event->description, event, event->refs_before_response);
	
	if (event->refs_before_response > 0)
		event->refs_before_response--;
	
	if (event->refs_before_response == 0)
		pthread_cond_broadcast(&event->response_cond);
	
	pthread_mutex_unlock(&event->mutex);
}

int crtx_wait_on_event(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	while (event->refs_before_response > 0)
		pthread_cond_wait(&event->response_cond, &event->mutex);
	
	pthread_mutex_unlock(&event->mutex);
	
	return event->error;
}

void crtx_invalidate_event(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	event->error = 1;
	event->refs_before_response = 0;
	pthread_mutex_unlock(&event->mutex);
	
	pthread_cond_broadcast(&event->response_cond);
}

void crtx_free_event(struct crtx_event *event) {
	// is somebody already releasing this event?
	if (!CAS(&event->release_in_progress, 0, 1)) {
		pthread_cond_broadcast(&event->release_cond);
		return;
	}
	
	// disappoint everyone who still waits for this event
	crtx_invalidate_event(event);
	
	// wait until all references are gone
	pthread_mutex_lock(&event->mutex);
	while (event->refs_before_release > 0)
		pthread_cond_wait(&event->release_cond, &event->mutex);
	pthread_mutex_unlock(&event->mutex);
	
	
	if (event->cb_before_release)
		event->cb_before_release(event, event->cb_before_release_data);
	
	crtx_free_dict_item_data(&event->data);
	crtx_free_dict_item_data(&event->response);
	
	free(event);
}

struct crtx_graph *crtx_find_graph_for_event_description(char *event_description) {
	unsigned int i, j;
	
	for (i=0; i < crtx_root->n_graphs; i++) {
		for (j=0; j < crtx_root->graphs[i]->n_descriptions; j++) {
			if (!strcmp(crtx_root->graphs[i]->descriptions[j], event_description)) {
				return crtx_root->graphs[i];
			}
		}
	}
	
	return 0;
}

struct crtx_graph *crtx_find_graph_for_event_type(CRTX_EVENT_TYPE_VARTYPE event_type) {
	unsigned int i, j;
	
	for (i=0; i < crtx_root->n_graphs; i++) {
		for (j=0; j < crtx_root->graphs[i]->n_types; j++) {
			if (crtx_root->graphs[i]->types[j] == event_type) {
				return crtx_root->graphs[i];
			}
		}
	}
	
	return 0;
}

struct crtx_graph *crtx_get_graph_for_event_description(char *event_description, char **new_event_types) {
	struct crtx_graph *graph;
	
	graph = crtx_find_graph_for_event_description(event_description);
	if (!graph) {
		crtx_create_graph(&graph, 0);
		if (new_event_types) {
			while (new_event_types[graph->n_types]) {
				graph->n_types++;
			}
			
			graph->descriptions = new_event_types;
		}
	}
	
	return graph;
}

int crtx_find_graph_add_event(struct crtx_event *event) {
	struct crtx_graph *graph;
	
	graph = 0;
	
	if (event->description)
		graph = crtx_find_graph_for_event_description(event->description);
	
	if (!graph)
		graph = crtx_find_graph_for_event_type(event->type);
	
	if (!graph) {
		CRTX_ERROR("did not find graph for event type \"%s\"\n", event->description);
		return -ENOENT;
	}
	
	crtx_add_event(graph, event);
	
	return 0;
}


int crtx_init_graph(struct crtx_graph *graph, const char *name) {
	int i, ret;
	
	ret = pthread_mutex_init(&graph->mutex, 0); CRTX_ASSERT(ret >= 0);
	
	ret = pthread_mutex_init(&graph->queue_mutex, 0); CRTX_ASSERT(ret >= 0);
	ret = pthread_cond_init(&graph->queue_cond, NULL); CRTX_ASSERT(ret >= 0);
	
	graph->name = name;
	
	LOCK(crtx_root->graphs_mutex);
	for (i=0; i < crtx_root->n_graphs; i++) {
		if (crtx_root->graphs[i] == 0) {
			crtx_root->graphs[i] = graph;
			break;
		}
	}
	if (i == crtx_root->n_graphs) {
		crtx_root->n_graphs++;
		crtx_root->graphs = (struct crtx_graph**) realloc(crtx_root->graphs, sizeof(struct crtx_graph*)*crtx_root->n_graphs);
		crtx_root->graphs[crtx_root->n_graphs-1] = graph;
	}
	UNLOCK(crtx_root->graphs_mutex);
	
	return 0;
}

int crtx_create_graph(struct crtx_graph **crtx_graph, const char *name) {
	struct crtx_graph *graph;
	
	graph = (struct crtx_graph*) calloc(1, sizeof(struct crtx_graph)); CRTX_ASSERT(graph);
	
	if (crtx_graph)
		*crtx_graph = graph;
	
	return crtx_init_graph(graph, name);
}

static int shutdown_graph_intern(struct crtx_graph *egraph, char crtx_shutdown) {
	size_t i;
	struct crtx_task *t, *tnext;
	struct crtx_dll *qe, *qe_next;
	
	CRTX_DBG("free graph %s t[0]: %d\n", egraph->name, egraph->types?egraph->types[0]:0);
	
	if (!crtx_shutdown) {
		LOCK(crtx_root->graphs_mutex);
		
		for (i=0; i < crtx_root->n_graphs; i++) {
			if (crtx_root->graphs[i] == egraph) {
				crtx_root->graphs[i] = 0;
				break;
			}
		}
		
		UNLOCK(crtx_root->graphs_mutex);
	}
	
	for (qe = egraph->equeue; qe; qe=qe_next) {
		crtx_free_event(qe->event);
		qe_next = qe->next;
		free(qe);
	}
	
	for (t = egraph->tasks; t; t=tnext) {
		tnext = t->next;
		crtx_free_task(t);
	}
	
	return 0;
}

int crtx_shutdown_graph(struct crtx_graph *egraph) {
	return shutdown_graph_intern(egraph, 0);
}

static int free_graph_intern(struct crtx_graph *egraph, char crtx_shutdown) {
	shutdown_graph_intern(egraph, crtx_shutdown);
	
	free(egraph);
	
	return 0;
}

int crtx_free_graph(struct crtx_graph *egraph) {
	return free_graph_intern(egraph, 0);
}

struct crtx_task *new_task() {
	struct crtx_task *etask = (struct crtx_task*) calloc(1, sizeof(struct crtx_task));
	etask->position = 100;
	
	return etask;
}

void crtx_free_task(struct crtx_task *task) {
	struct crtx_task *t, *prev;
	
	CRTX_DBG("free task %s\n", task->id);
	
	prev=0;
	for (t = task->graph->tasks; t; t=t->next) {
		if (t == task) {
			if (prev) {
				prev->next = t->next;
			} else {
				task->graph->tasks = t->next;
			}
		}
		prev = t;
	}
	
	free(task);
}

/// release all remaining events in all graphs
void crtx_flush_events() {
	size_t i;
	
	for (i=0; i<crtx_root->n_graphs; i++) {
		struct crtx_dll *qe, *qe_next;
		
		if (!crtx_root->graphs[i])
			continue;
		
		LOCK(crtx_root->graphs[i]->queue_mutex);
		
		for (qe = crtx_root->graphs[i]->equeue; qe ; qe = qe_next) {
			qe_next = qe->next;
			
			crtx_invalidate_event(qe->event);
			
			crtx_dereference_event_response(qe->event);
			crtx_dereference_event_release(qe->event);
			
			crtx_dll_unlink(&crtx_root->graphs[i]->equeue, qe);
		}
		
		UNLOCK(crtx_root->graphs[i]->queue_mutex);
	}
}

/// start the main event loop
void crtx_loop() {
	if (!crtx_root->event_loop)
		return;
	
	// the thread calling this either executes the event loop or becomes one of
	// the worker threads in the thread pool
	if (crtx_root->event_loop->listener) {
		crtx_evloop_start(crtx_root->event_loop);
	} else {
		spawn_thread(0);
	}
	
	CRTX_DBG("crtx_loop() end\n");
	
	if (crtx_root->reinit_after_shutdown) {
		crtx_finish();
		
		crtx_init();
		
		if (crtx_root->reinit_cb) {
			crtx_root->reinit_cb(crtx_root->reinit_cb_data);
			crtx_root->reinit_cb = 0;
		}
		
		crtx_loop();
	}
}

int crtx_loop_onetime() {
	return crtx_root->event_loop->onetime(crtx_root->event_loop);
}

static void *evloop_detached_main(void *data) {
	crtx_evloop_start(crtx_root->event_loop);
	
	return 0;
}

int crtx_separate_evloop_thread() {
	struct crtx_thread *t;
	
	if (!crtx_root->evloop_thread)
		return 0;
	
	t = crtx_get_own_thread();
	
	if (t == crtx_root->evloop_thread)
		return 0;
	
	return 1;
}

struct crtx_thread * crtx_start_detached_event_loop() {
	if (!crtx_root->evloop_thread) {
		if (!crtx_root->event_loop)
			crtx_get_main_event_loop();
		
		crtx_root->evloop_thread = crtx_thread_assign_job(&crtx_root->evloop_job);
		
		crtx_reference_signal(&crtx_root->evloop_thread->finished);
		
		crtx_thread_start_job(crtx_root->evloop_thread);
	}
	
	return crtx_root->evloop_thread;
}

void crtx_init_shutdown() {
	if (crtx_root->shutdown)
		return;
	
	CRTX_VDBG("init shutdown\n");
	
	crtx_root->shutdown = 1;
	
	struct crtx_ll *ll, *ll_next;
	for (ll=crtx_root->listeners; ll; ll=ll_next) {
		ll_next = ll->next;
		
		// TODO see fork.test
		if (!ll->data)
			continue;
		
		if (crtx_root->event_loop && crtx_root->event_loop->listener == ll->data) {
			CRTX_VDBG("skip eloop listener\n");
			continue;
		}
		
		crtx_stop_listener((struct crtx_listener_base *) ll->data);
	}
	
	if (crtx_root->event_loop)
		crtx_root->event_loop->phase_out = 1;
}

void crtx_shutdown_after_fork() {
	// after a fork, we close all file handles but we don't want to remove
	// them from an epoll instance as it would remove the file handle for
	// the parent process as well.
	//
	// this also causes the child eventloop to stop processing further events
	
	crtx_root->event_loop->after_fork_close = 1;
	
	// we do not process the events in the queue
	crtx_flush_events();
	
	crtx_evloop_stop(crtx_root->event_loop);
	
	// close all file descriptors here as we must not send anything anymore especially
	// no "goodbye" messages to network peers
	struct crtx_ll *lit = crtx_root->listeners;
	struct crtx_listener_base* lstnr;
	while (lit) {
		lstnr = (struct crtx_listener_base*) lit->data;
		if (lstnr && lstnr->evloop_fd.fd >= 0) {
			crtx_evloop_disable_cb(&lstnr->default_el_cb);
		}
		
		lit = lit->next;
	}
	
	// create a new event loop that handles shutdown
	crtx_root->event_loop = 0;
	crtx_get_main_event_loop();
	
	crtx_init_shutdown();
}

struct crtx_listener_repository* crtx_get_new_listener_repo_entry() {
	crtx_root->listener_repository_length++;
	crtx_root->listener_repository = (struct crtx_listener_repository*) realloc(crtx_root->listener_repository, sizeof(struct crtx_listener_repository)*(crtx_root->listener_repository_length+1));
	memset(&crtx_root->listener_repository[crtx_root->listener_repository_length], 0, sizeof(struct crtx_listener_repository));
	
	return &crtx_root->listener_repository[crtx_root->listener_repository_length-1];
}

#include <dlfcn.h>
#include <dirent.h>
static void load_plugin(char *path, char *basename) {
	struct crtx_lstnr_plugin *p;
	struct crtx_listener_repository *lrepo;
	char buf[1024];
	char plugin_name[900];
	size_t len;
	char *dot;
	
	CRTX_DBG("load plugin \"%s\"\n", path);
	
	
	len = strlen(basename);
	if (len <= 12)
		return;
	
	dot = strchr(basename, '.');
	if (dot == 0 || dot - basename <= 8)
		return;
	
	memcpy(plugin_name, basename+8, dot - basename - 8);
	plugin_name[dot - basename - 8] = 0;
	
	for (len = 0; len < crtx_root->n_plugins; len++) {
		if (!strcmp(plugin_name, crtx_root->plugins[len].plugin_name)) {
			CRTX_VDBG("ignoring \"%s\", already loaded\n", plugin_name);
			return;
		}
	}
	
	crtx_root->n_plugins++;
	crtx_root->plugins = (struct crtx_lstnr_plugin*) realloc(crtx_root->plugins, sizeof(struct crtx_lstnr_plugin)*crtx_root->n_plugins);
	p = &crtx_root->plugins[crtx_root->n_plugins-1];
	
	memset(p, 0, sizeof(struct crtx_lstnr_plugin));
	p->path = path;
	p->basename = basename;
	p->handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
	p->plugin_name = strdup(plugin_name);
	
	if (!p->handle) {
		CRTX_ERROR("dlopen(%s) failed: %s\n", path, dlerror());
		return;
	}
	
	p->initialized = 1;
	snprintf(buf, 1024, "crtx_%s_init", plugin_name);
	p->init = dlsym(p->handle, buf);
	snprintf(buf, 1024, "crtx_%s_finish", plugin_name);
	p->finish = dlsym(p->handle, buf);
	snprintf(buf, 1024, "crtx_%s_get_listener_factory", plugin_name);
	p->get_listener_repository = dlsym(p->handle, buf);
	
	if (p->init)
		p->initialized = p->init();
	
	if (p->get_listener_repository) {
		p->get_listener_repository(&crtx_root->listener_repository, &crtx_root->listener_repository_length);
	} else {
		char **crtx_listener_list;
		void *create;
		
		snprintf(buf, 1024, "crtx_new_%s_listener", plugin_name);
		create = dlsym(p->handle, buf);
		
		if (!create) {
			snprintf(buf, 1024, "crtx_%s_new_listener", plugin_name);
			create = dlsym(p->handle, buf);
		}
		
		if (!create) {
			snprintf(buf, 1024, "crtx_setup_%s_listener", plugin_name);
			create = dlsym(p->handle, buf);
		}
		
		if (create) {
			lrepo = crtx_get_new_listener_repo_entry();
			
			lrepo->id = crtx_stracpy(plugin_name, 0);
			lrepo->create = create;
		}
		
		crtx_listener_list = (char**) dlsym(p->handle, "crtx_listener_list");
		
		if (crtx_listener_list) {
			char **s;
			
			s = crtx_listener_list;
			while (*s) {
				snprintf(buf, 1024, "crtx_new_%s_listener", *s);
				create = dlsym(p->handle, buf);
				
				if (create) {
					lrepo = crtx_get_new_listener_repo_entry();
					
					lrepo->id = *s;
					lrepo->create = create;
				}
				s++;
			}
		}
	}
}

static void load_plugins(char * directory) {
	DIR *dhandle;
	struct dirent *dent;
	struct stat buf;
	int result;
	
	
	if (!directory)
		return;
	
	dhandle = opendir(directory);
	if (dhandle != NULL) {
		while ((dent = readdir(dhandle)) != NULL) {
			size_t len;
			char * fpath;
			
			
			fpath = (char*) malloc(strlen(directory)+strlen(dent->d_name)+2);
			sprintf(fpath, "%s/%s", directory, dent->d_name);
			
			result = lstat(fpath, &buf);
			
			len = strlen(dent->d_name);
			if (len > strlen(SO_VERSION))
				len -= strlen(SO_VERSION);
			
			if (
				!strncmp(dent->d_name, "libcrtx_", 8) &&
				!strncmp(&dent->d_name[len], SO_VERSION, strlen(SO_VERSION)) &&
				(result != 0 || !S_ISLNK(buf.st_mode)))
			{
				load_plugin(fpath, dent->d_name);
			} else {
				free(fpath);
			}
		}
		closedir(dhandle);
	} else {
		printf("cannot open plugin directory \"%s\"\n", directory);
	}
	
	return;
}

// TODO
#if 0
#include <sys/types.h>
#include <sys/stat.h>
void crtx_daemonize() {
	pid_t pid = 0;
	int r, fd;
	
	signal(SIGCHLD, SIG_IGN);
	
	pid = fork();
	if (pid < 0) {
		CRTX_ERROR("fork failed\n");
		exit(1);
	}
	if (pid > 0) {
		CRTX_DBG("parent exits\n");
		exit(0);
	}
	
	r = setsid();
	if (r < 0) {
		CRTX_ERROR("setsid failed: %d\n", r);
		exit(1);
	}
	
	umask(0);
	chdir("/");
	
	for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--) {
		close(fd);
	}
	fclose(stdin);
	
	stdin = fopen("/dev/null", "r");
	stdout = fopen("/dev/null", "w+");
	stderr = fopen("/dev/null", "w+");
}
#endif

int crtx_init() {
	unsigned int i;
	
	
	if (crtx_root->initialized)
		return 0;
	
	crtx_root->initialized = 1;
	
	if (getenv("CRTX_VERBOSITY"))
		crtx_verbosity = atoi(getenv("CRTX_VERBOSITY"));
	
	if (getenv("CRTX_PID_PREFIX"))
		crtx_pid_prefix = atoi(getenv("CRTX_PID_PREFIX"));
	
	CRTX_DBG("initializing libcortex (PID: %d)\n", getpid());
	
	crtx_root->shutdown = 0;
	crtx_root->default_mode = CRTX_PREFER_ELOOP;
	crtx_root->global_fd_flags = O_CLOEXEC;
	
	if (crtx_root->event_loop)
		crtx_root->event_loop->after_fork_close = 0;
	crtx_root->reinit_after_shutdown = 0;
	
	crtx_root->evloop_job.fct = &evloop_detached_main;
	
	INIT_MUTEX(crtx_root->graphs_mutex);
	
	crtx_threads_init();
	
	i=0;
	while (static_modules[i].id) {
		CRTX_DBG("initialize \"%s\"\n", static_modules[i].id);
		
		static_modules[i].init();
		i++;
	}
	
	if (getenv("CRTX_PLUGIN_DIR")) {
		load_plugins(getenv("CRTX_PLUGIN_DIR"));
	} else {
		load_plugins(CRTX_PLUGIN_DIR);
	}
	
	return 0;
}

int crtx_finish() {
	unsigned int i;
	struct crtx_ll *it, *itn;
	
	if (!crtx_root->initialized)
		return 0;
	
	crtx_root->initialized = 0;
	
	crtx_init_shutdown();
	
	CRTX_VDBG("crtx_finish()\n");
	
	crtx_threads_stop_all();
	crtx_flush_events();
	
	for (it = crtx_root->listeners; it; it = itn) {
		itn = it->next;
		
		if (crtx_root->event_loop && crtx_root->event_loop->listener == it->data) {
			CRTX_VDBG("skip eloop listener\n");
			continue;
		}
		
		crtx_shutdown_listener((struct crtx_listener_base*) it->data);
	}
	
	if (crtx_root->evloop_thread) {
		crtx_evloop_stop(crtx_root->event_loop);
		crtx_threads_stop(crtx_root->evloop_thread);
		
		crtx_wait_on_signal(&crtx_root->evloop_thread->finished, 0);
		crtx_dereference_signal(&crtx_root->evloop_thread->finished);
	}
	
	while (crtx_root->event_loops) {
		crtx_evloop_stop((struct crtx_event_loop*) crtx_root->event_loops);
		crtx_evloop_release((struct crtx_event_loop*) crtx_root->event_loops);
	}
	crtx_root->event_loop = 0;
	
	i=0;
	while (static_modules[i].id) { i++; }
	i--;
	
	crtx_threads_stop_all();
	
	while (i > 0) {
		CRTX_DBG("finish \"%s\"\n", static_modules[i].id);
		
		static_modules[i].finish();
		i--;
	}
	
	crtx_threads_finish();
	
	LOCK(crtx_root->graphs_mutex);
	for (i=0; i < crtx_root->n_graphs; i++) {
		if (crtx_root->graphs[i])
			free_graph_intern(crtx_root->graphs[i], 1);
	}
	free(crtx_root->graphs);
	crtx_root->graphs = 0;
	crtx_root->n_graphs = 0;
	UNLOCK(crtx_root->graphs_mutex);
	
	for (i=0; i < crtx_root->n_plugins; i++) {
		CRTX_DBG("finish \"%s\"\n", crtx_root->plugins[i].plugin_name);
		
		if (crtx_root->plugins[i].finish)
			crtx_root->plugins[i].finish();
		
		if (crtx_root->plugins[i].handle) {
			dlclose(crtx_root->plugins[i].handle);
		}
		
		free(crtx_root->plugins[i].path);
		free(crtx_root->plugins[i].plugin_name);
	}
	free(crtx_root->plugins);
	crtx_root->plugins = 0;
	crtx_root->n_plugins = 0;
	
	if (crtx_root->selfpipe_lstnr)
		free(crtx_root->selfpipe_lstnr);
	
	CRTX_DBG("finished shutdown\n");
	
	return 0;
}

void crtx_print_tasks(struct crtx_graph *graph) {
	struct crtx_task *e;
	
	for (e=graph->tasks; e; e=e->next) {
		CRTX_INFO("%u %s %s\n", e->position, e->id, e->event_type_match);
	}
}

void crtx_hexdump(unsigned char *buffer, size_t index) {
	size_t i;
	
	for (i=0;i<index;i++) {
		CRTX_INFO("%02x ", buffer[i]);
	}
	CRTX_INFO("\n");
}

void crtx_event_set_raw_data(struct crtx_event *event, unsigned char type, ...) {
	struct crtx_dict *upgraded_dict;
	struct crtx_dict_item *di;
	va_list va;
	
	
	LOCK(event->mutex);
	
	va_start(va, type);

	if (event->data.type == 'D') {
		if (event->data.key && !strcmp(event->data.key, "crtx_raw_data")) {
			di = crtx_get_item_by_idx(event->data.dict, 0);
			crtx_fill_data_item_va2(di, type, "crtx_raw_data", &va);
		} else {
			upgraded_dict = crtx_init_dict(0, 2, 0);
			
			di = crtx_get_item_by_idx(upgraded_dict, 0);
			
			crtx_fill_data_item_va2(di, type, "crtx_raw_data", &va);
			
			di = crtx_get_item_by_idx(upgraded_dict, 1);
			memcpy(di, &event->data, sizeof(struct crtx_dict_item));
			
			event->data.key = "crtx_dict_data";
			event->data.type = 'D';
			event->data.size = 0;
			event->data.flags = 0;
			event->data.dict = upgraded_dict;
		}
	} else {
		crtx_fill_data_item_va2(&event->data, type, "crtx_raw_data", &va);
	}
	
	va_end(va);
	
	UNLOCK(event->mutex);
}

void crtx_event_set_dict_data(struct crtx_event *event, struct crtx_dict *data_dict, unsigned char n_additional_fields) {
	struct crtx_dict *upgraded_dict;
	struct crtx_dict_item *di;
	
	LOCK(event->mutex);
	
	if (event->data.type != 'D') {
		upgraded_dict = crtx_init_dict(0, 2+n_additional_fields, 0);
		
		di = crtx_get_item_by_idx(upgraded_dict, 0);
		
		memcpy(di, &event->data, sizeof(struct crtx_dict_item));
		
		di = crtx_get_item_by_idx(upgraded_dict, 1);
		di->key = "crtx_dict_data";
		di->type = 'D';
		di->dict = data_dict;
		
		event->data.key = "crtx_dict_data";
		event->data.type = 'D';
		event->data.size = 0;
		event->data.flags = 0;
		event->data.dict = upgraded_dict;
	} else
	if (event->data.type == 'D') {
		if (!strcmp(event->data.key, "crtx_dict_data")) {
			di = crtx_get_item_by_idx(event->data.dict, 1);
			di->dict = data_dict;
			
			if (event->data.dict->signature_length < 2 + n_additional_fields)
				crtx_resize_dict(event->data.dict, 2 + n_additional_fields);
		} else {
			event->data.dict = data_dict;
			if (n_additional_fields > 0)
				CRTX_ERROR("TODO\n");
		}
	}
	
	UNLOCK(event->mutex);
}

void crtx_event_set_dict(struct crtx_event *event, char *signature, ...) {
	struct crtx_dict *dict;
	va_list va;
	
	
	va_start(va, signature);
	
	dict = crtx_create_dict_va(signature, &va);
	
	LOCK(event->mutex);
	
	crtx_fill_data_item(&event->data, 'D', 0, dict);
	
	UNLOCK(event->mutex);
	
	va_end(va);
}

int crtx_event_raw2dict(struct crtx_event *event, void *user_data) {
	struct crtx_dict_item *raw2dict;
	crtx_raw_to_dict_t raw2dict_fct;
	
	if (event->data.type != 'D')
		return -1;
	
	raw2dict = crtx_get_item(event->data.dict, "raw2dict");
	
	if (!raw2dict)
		return -1;
	
	if (raw2dict->type != 'p')
		return -1;
	
	raw2dict_fct = (crtx_raw_to_dict_t) raw2dict->pointer;
	
	raw2dict_fct(event, 0, user_data);
	
	return CRTX_SUCCESS;
}

void crtx_event_get_payload(struct crtx_event *event, char *id, void **raw_pointer, struct crtx_dict **dict) {
	struct crtx_dict_item *di;
	
	if (raw_pointer) {
		if (event->data.type == 'p') {
			*raw_pointer = event->data.pointer;
		} else
		if (event->data.type == 'D') {
			if (!strcmp(event->data.key, "crtx_dict_data")) {
				di = crtx_get_item_by_idx(event->data.dict, 0);
				*raw_pointer = di->pointer;
			} else {
				CRTX_ERROR("unexpected event dict %p\n", event->data.dict);
				*raw_pointer = 0;
			}
		} else {
			CRTX_ERROR("unexpected event data type %d\n", event->data.type);
			*raw_pointer = 0;
		}
	}
	if (dict) {
		if (event->data.type != 'D') {
			int ret;
			
			ret = crtx_event_raw2dict(event, 0);
			if (ret != CRTX_SUCCESS) {
				CRTX_ERROR("cannot convert raw data (%c)\n", event->data.type);
				*dict = 0;
				return;
			}
		}
		
		if (!strcmp(event->data.key, "crtx_dict_data")) {
			di = crtx_get_item_by_idx(event->data.dict, 1);
			*dict = di->dict;
		} else {
			*dict = event->data.dict;
		}
	}
}

struct crtx_dict *crtx_event_get_dict(struct crtx_event *event) {
	struct crtx_dict *dict;
	
	crtx_event_get_payload(event, 0, 0, &dict);
	if (!dict) {
		return 0;
	}
	
	return dict;
}

struct crtx_dict_item *crtx_event_get_item_by_key(struct crtx_event *event, char *id, char *key) {
	struct crtx_dict *dict;
	
	crtx_event_get_payload(event, id, 0, &dict);
	
	if (!dict)
		return 0;
	
	return crtx_get_item(dict, key);
}

int crtx_event_get_value_by_key(struct crtx_event *event, char *key, char type, void *buffer, size_t buffer_size) {
	struct crtx_dict *dict;
	int r;
	
	crtx_event_get_payload(event, 0, 0, &dict);
	if (!dict) {
		return -1;
	}
	
	r = crtx_get_value(dict, key, type, buffer, buffer_size);
	if (r < 0) {
		return r;
	}
	
	return 0;
}

void *crtx_event_get_ptr(struct crtx_event *event) {
	void *ptr;
	
	crtx_event_get_payload(event, 0, &ptr, 0);
	
	return ptr;
}

char *crtx_event_get_string(struct crtx_event *event, char *key) {
	struct crtx_dict *dict;
	
	crtx_event_get_payload(event, 0, 0, &dict);
	if (!dict) {
		return 0;
	}
	
	return crtx_get_string(dict, key);
}

int crtx_event_get_int(struct crtx_event *event, int *value) {
	if (event->data.type == 'i') {
		*value = event->data.int32;
	} else {
		return EINVAL;
	}
	
	return 0;
}

int crtx_event_get_data(struct crtx_event *event, void **data, size_t *size) {
	if (event->data.type != 'p')
		return -EINVAL;
	if (!data)
		return -EINVAL;
	
	*data = event->data.pointer;
	*size = event->data.size;
	
	return 0;
}

void crtx_register_handler_for_event_type(char *event_type, char *handler_name, crtx_handle_task_t handler_function, void *handler_data) {
	struct crtx_ll *catit, *entry;
	
	for (catit = crtx_root->handler_categories; catit && strcmp(catit->handler_category->event_type, event_type); catit=catit->next) {}
	
	if (!catit) {
		catit = crtx_ll_append_new(&crtx_root->handler_categories, 0);
		catit->handler_category = 0;
	}
	
	if (!catit->handler_category) {
		catit->handler_category = (struct crtx_handler_category *) malloc(sizeof(struct crtx_handler_category));
		catit->handler_category->event_type = event_type;
		catit->handler_category->entries = 0;
	}
	
	entry = crtx_ll_append_new(&catit->handler_category->entries, 0);
	
	entry->handler_category_entry = (struct crtx_handler_category_entry *) malloc(sizeof(struct crtx_handler_category_entry));
	entry->handler_category_entry->handler_name = handler_name;
	entry->handler_category_entry->function = handler_function;
	entry->handler_category_entry->handler_data = handler_data;
	
	CRTX_VDBG("new handler %s for etype %s\n", handler_name, event_type);
}

void crtx_set_main_event_loop(const char *event_loop) {
	crtx_root->chosen_event_loop = event_loop;
}

// DEPRECATED
void crtx_lock_listener_source(struct crtx_listener_base *lbase) {
	LOCK(lbase->source_lock);
}

// DEPRECATED
void crtx_unlock_listener_source(struct crtx_listener_base *lbase) {
	UNLOCK(lbase->source_lock);
}

void crtx_lock_listener(struct crtx_listener_base *lbase) {
	LOCK(lbase->source_lock);
}

void crtx_unlock_listener(struct crtx_listener_base *lbase) {
	UNLOCK(lbase->source_lock);
}

int crtx_handle_std_signals() {
	return crtx_signals_handle_std_signals(0);
}

void crtx_trigger_event_processing(struct crtx_listener_base *lstnr) {
	// for example, a client might want to trigger a write before the writequeue
	// has been started for the first time
	if (!lstnr->evloop_fd.evloop) {
		lstnr->evloop_fd.evloop = crtx_get_main_event_loop();
		if (!lstnr->evloop_fd.evloop) {
			CRTX_ERROR("no event loop for fd %d\n", *lstnr->evloop_fd.fd);
			return;
		}
	}
	
	crtx_evloop_trigger_callback(lstnr->evloop_fd.evloop, &lstnr->default_el_cb);
}

void crtx_trigger_event_callback(struct crtx_evloop_callback *evloop_cb) {
	crtx_evloop_trigger_callback(evloop_cb->fd_entry->evloop, evloop_cb);
}

int crtx_get_version(unsigned int *major, unsigned int *minor, unsigned int *revision) {
	if (!major || !minor)
		return -EINVAL;
	
	*major = MAJOR_VERSION;
	*minor = MINOR_VERSION;
	if (revision)
		*revision = REVISION;
	
	return 0;
}

#if CRTX_HAVE_MOD_PIPE
void crtx_selfpipe_cb_free_lstnr(void *data) {
	struct crtx_listener_base *lstnr;
	
	lstnr = (struct crtx_listener_base *) data;
	
	CRTX_DBG("selfpipe free listener %p\n", lstnr);
	
	crtx_shutdown_listener(lstnr);
}

static int selfpipe_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_pipe_listener *selfpipe_lstnr;
	ssize_t r;
	void (*cb)(void*);
	void *cb_data;
	char value;
	
	
	selfpipe_lstnr = (struct crtx_pipe_listener *) crtx_root->selfpipe_lstnr;
	
	r = read(selfpipe_lstnr->fds[0], &value, sizeof(value));
	if (r < 0) {
		CRTX_ERROR("read from selfpipe %d failed: %s\n", selfpipe_lstnr->fds[0], strerror(r));
		return r;
	}
	if (r != sizeof(value)) {
		CRTX_ERROR("read invalid data from signal selfpipe %d: %zd != %zd\n", selfpipe_lstnr->fds[0], r, sizeof(value));
		return -EBADE;
	}
	
	if (value != 1) {
		CRTX_ERROR("received unexpected value over selfpipe: %d\n", value);
		return -EBADE;
	}
	
	r = read(selfpipe_lstnr->fds[0], &cb, sizeof(void*));
	if (r < 0) {
		CRTX_ERROR("read from selfpipe %d failed: %s\n", selfpipe_lstnr->fds[0], strerror(r));
		return r;
	}
	if (r != sizeof(void*)) {
		CRTX_ERROR("read invalid data from signal selfpipe %d: %zd != %zd\n", selfpipe_lstnr->fds[0], r, sizeof(void*));
		return -EBADE;
	}
	
	r = read(selfpipe_lstnr->fds[0], &cb_data, sizeof(void*));
	if (r < 0) {
		CRTX_ERROR("read from selfpipe %d failed: %s\n", selfpipe_lstnr->fds[0], strerror(r));
		return r;
	}
	if (r != sizeof(void*)) {
		CRTX_ERROR("read invalid data from signal selfpipe %d: %zd != %zd\n", selfpipe_lstnr->fds[0], r, sizeof(void*));
		return -EBADE;
	}
	
	cb(cb_data);
	
	return 0;
}

static int selfpipe_setup(struct crtx_root *root) {
	if (!crtx_root->selfpipe_lstnr) {
		struct crtx_pipe_listener *selfpipe_lstnr;
		int rv;
		
		crtx_root->selfpipe_lstnr = (struct crtx_listener_base *) calloc(1, sizeof(struct crtx_pipe_listener));
		selfpipe_lstnr = (struct crtx_pipe_listener *) crtx_root->selfpipe_lstnr;
		
		crtx_pipe_clear_lstnr(selfpipe_lstnr);
		selfpipe_lstnr->fd_event_handler = &selfpipe_event_handler;
		
		rv = crtx_setup_listener("pipe", crtx_root->selfpipe_lstnr);
		if (rv) {
			CRTX_ERROR("create_listener(selfpipe) failed\n");
			return rv;
		}
		
		rv = crtx_start_listener(crtx_root->selfpipe_lstnr);
		if (rv) {
			CRTX_ERROR("starting selfpipe listener failed\n");
			return rv;
		}
	}
	
	return 0;
}

int crtx_selfpipe_enqueue_cb(void (*cb)(void*), void *cb_data) {
	ssize_t rv;
	char value;
	
	
	selfpipe_setup(crtx_root);
	
	value = 1;
	rv = write(((struct crtx_pipe_listener *) crtx_root->selfpipe_lstnr)->fds[1], &value, sizeof(value));
	if (rv != sizeof(value)) {
		CRTX_ERROR("write1 to selfpipe failed: %zd %zu\n", rv, sizeof(value));
		return 1;
	}
	
	rv = write(((struct crtx_pipe_listener *) crtx_root->selfpipe_lstnr)->fds[1], &cb, sizeof(void*));
	if (rv != sizeof(void*)) {
		CRTX_ERROR("write2 to selfpipe failed: %zd %zu\n", rv, sizeof(void*));
		return 1;
	}
	
	rv = write(((struct crtx_pipe_listener *) crtx_root->selfpipe_lstnr)->fds[1], &cb_data, sizeof(void*));
	if (rv != sizeof(void*)) {
		CRTX_ERROR("write3 to selfpipe failed: %zd %zu\n", rv, sizeof(void*));
		return 1;
	}
	
	return 0;
}
#endif

void crtx_lstnr_handle_fd_closed(struct crtx_listener_base *lstnr) {
	crtx_evloop_handle_fd_closed(&lstnr->evloop_fd);
}

void crtx_print_event(struct crtx_event *event, FILE *f) {
	if (!f)
		f = stdout;
	
	if (event->type)
		fprintf(f, "type: %d\n", event->type);
	if (event->description)
		fprintf(f, "desc: %s\n", event->description);
	
	crtx_dict_item_print(&event->data, 0, f);
}

int crtx_listener_add_fd(struct crtx_listener_base *listener,
						int *fd_ptr,
						int fd,
						int event_flags,
						uint64_t timeout_us,
						crtx_handle_task_t event_handler,
						void *event_handler_data,
						crtx_evloop_error_cb_t error_cb,
						void *error_cb_data
						)
{
	int rv;
	
	rv = crtx_evloop_init_listener(listener,
			fd_ptr,
			fd,
			event_flags,
			listener->graph,
			event_handler,
			event_handler_data,
			error_cb,
			error_cb_data
		);
	if (rv < 0) {
		return rv;
	}
	
	if (timeout_us >= 1)
		crtx_evloop_set_timeout_rel(&listener->default_el_cb, timeout_us - 1);
	
	return 0;
}

int crtx_listener_get_fd(struct crtx_listener_base *listener) {
	return *listener->evloop_fd.fd;
}

int crtx_is_shutting_down() {
	return crtx_root->shutdown;
}

int crtx_is_fd_valid(int fd) {
	return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}



