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
 
// for get_username()
#include <unistd.h>
#include <pwd.h>

#include <fcntl.h>

#include "intern.h"
#include "core.h"
#include "dict.h"


#ifndef CRTX_PLUGIN_DIR
#define CRTX_PLUGIN_DIR "/usr/lib/cortexd/plugins/"
#endif

struct crtx_root crtx_global_root = { 0 };
struct crtx_root *crtx_root = &crtx_global_root;

#include "core_modules.h"

char * crtx_evt_control[] = { CRTX_EVT_MOD_INIT, CRTX_EVT_SHUTDOWN, 0 };
// char *crtx_evt_notification[] = { CRTX_EVT_NOTIFICATION, 0 };
char *crtx_evt_inbox[] = { CRTX_EVT_INBOX, 0 };
char *crtx_evt_outbox[] = { CRTX_EVT_OUTBOX, 0 };

char crtx_verbosity = CRTX_VLEVEL_INFO;


char *crtx_stracpy(const char *str, size_t *str_length) {
	char *r;
	size_t length;
	
	if (!str) {
		INFO("trying to stracpy null pointer\n");
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
		fprintf(stderr, "[%d] ", getpid());
		vfprintf(stderr, format, va);
	} else {
		if (level <= crtx_verbosity) {
			fprintf(stdout, "[%d] ", getpid());
			vfprintf(stdout, format, va);
			fflush(stdout);
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
	
// 	crtx_stop_listener(listener);
	crtx_free_listener(listener);
}

int crtx_start_listener(struct crtx_listener_base *listener) {
	enum crtx_processing_mode mode;
	struct crtx_event *event;
	int ret;
	
	LOCK(listener->state_mutex);
	
	if (listener->state == CRTX_LSTNR_STARTED) {
		DBG("listener already started\n");
		UNLOCK(listener->state_mutex);
		return -EEXIST;
	}
	
// 	if (listener->evloop_fd.fd < 0 && !listener->thread_job.fct && !listener->start_listener) {
// 		DBG("no method to start listener \"%s\" provided (%d, %p, %p, %d)\n", listener->id, listener->evloop_fd.fd, listener->thread_job.fct, listener->start_listener, listener->mode);
// 		
// 		UNLOCK(listener->state_mutex);
// 		return -EINVAL;
// 	}
	
	listener->state = CRTX_LSTNR_STARTING;
	
	DBG("starting listener \"%s\"", listener->id);
	if (listener->evloop_fd.fd >=0)
		DBG(" (fd %d)", listener->evloop_fd.fd);
	DBG("\n");
	
	if (listener->start_listener) {
		ret = listener->start_listener(listener);
		if (ret < 0 && listener->state_graph) {
			ERROR("start_listener failed\n");
			
			listener->state = CRTX_LSTNR_STOPPED;
			
			ret = crtx_create_event(&event);
			if (ret) {
				ERROR("crtx_create_event failed: %s\n", strerror(ret));
			} else {
				event->description = "listener_state";
				event->data.type = 'u';
				event->data.uint32 = CRTX_LSTNR_STOPPED;
				
				crtx_add_event(listener->state_graph, event);
			}
			
			UNLOCK(listener->state_mutex);
			
			return ret;
		}
	}
	
	// start_listener() might modify listener->mode
	mode = crtx_get_mode(listener->mode);
	
	if (listener->evloop_fd.fd < 0 && !listener->thread_job.fct && !listener->start_listener && mode != CRTX_NO_PROCESSING_MODE) {
		DBG("no method to start listener \"%s\" provided (%d, %p, %p, %d)\n", listener->id, listener->evloop_fd.fd, listener->thread_job.fct, listener->start_listener, listener->mode);
		
		UNLOCK(listener->state_mutex);
		return -EINVAL;
	}
	
	if (listener->evloop_fd.fd || listener->thread_job.fct) {
		if (mode == CRTX_PREFER_ELOOP && listener->evloop_fd.fd <= 0) {
			ERROR("listener mode set to \"event loop\" but no event loop data available (fd = %d)\n", listener->evloop_fd.fd);
			mode = CRTX_PREFER_THREAD;
		}
		if (mode == CRTX_PREFER_THREAD && !listener->thread_job.fct) {
			ERROR("listener mode set to \"thread\" but no thread data available\n");
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
			
			crtx_evloop_add_el_fd(&listener->evloop_fd);
		} else
		if (mode == CRTX_NO_PROCESSING_MODE) {
			// do nothing
		} else {
			ERROR("invalid start listener mode: %d\n", mode);
			UNLOCK(listener->state_mutex);
			return -EINVAL;
		}
	}
	
	listener->state = CRTX_LSTNR_STARTED;
	
	if (listener->state_graph) {
		ret = crtx_create_event(&event);
		if (ret) {
			ERROR("crtx_create_event failed: %s\n", strerror(ret));
		} else {
			event->description = "listener_state";
			event->data.type = 'u';
			event->data.uint32 = CRTX_LSTNR_STARTED;
			crtx_add_event(listener->state_graph, event);
		}
	}
	
	if (listener->flags & CRTX_LSTNR_STARTUP_TRIGGER)
		crtx_trigger_event_processing(listener);
	
	UNLOCK(listener->state_mutex);
	
	return 0;
}

int crtx_update_listener(struct crtx_listener_base *listener) {
	if (listener->state != CRTX_LSTNR_STARTED) {
		return crtx_start_listener(listener);
	}
	
	if (listener->update_listener) {
		return listener->update_listener(listener);
	}
	
	return 0;
}

// static void listener_thread_on_finish(struct crtx_thread *thread, void *userdata) {
// 	free_listener( (struct crtx_listener_base*) userdata);
// }

int crtx_init_listener_base(struct crtx_listener_base *lbase) {
// 	lbase->id = id;
	
	INIT_MUTEX(lbase->state_mutex);
	
	lbase->state = CRTX_LSTNR_STOPPED;
	
	if (!lbase->graph) {
		crtx_create_graph(&lbase->graph, lbase->id);
		lbase->graph->listener = lbase;
	}
	
	INIT_REC_MUTEX(lbase->source_lock);
	
	lbase->evloop_fd.fd = -1;
	
	return 0;
}

struct crtx_listener_base *create_listener(const char *id, void *options) {
	struct crtx_listener_repository *l;
	struct crtx_listener_base *lbase;
	char static_lstnr;
	
	DBG("new listener %s\n", id);
	
	static_lstnr = 1;
	lbase = 0;
	l = static_listener_repository;
	while (l && l->id) {
		while (l->id) {
			if (!strcmp(l->id, id)) {
				crtx_init_listener_base( (struct crtx_listener_base *) options );
				
				lbase = l->create(options);
				
				if (!lbase) {
					ERROR("creating listener \"%s\" failed: create procedure failed\n", id);
					return 0;
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
		ERROR("creating listener \"%s\" failed: not found\n", id);
		return 0;
	}
	
	lbase->id = id;
	
	LOCK(crtx_root->listeners_mutex);
	lbase->ll.data = lbase;
	crtx_ll_append(&crtx_root->listeners, &lbase->ll);
	UNLOCK(crtx_root->listeners_mutex);
	
	return lbase;
}

int crtx_create_listener(const char *id, void *options) {
	struct crtx_listener_base *lbase;
	
	
	if (id == 0)
		return -EINVAL;
	
	lbase = create_listener(id, options);
	
// 	if (listener)
// 		*listener = lbase;
	
	return (lbase == 0);
}

int crtx_stop_listener(struct crtx_listener_base *listener) {
// 	if (listener->state != CRTX_LSTNR_STARTED && listener != CRTX_LSTNR_PAUSED) {
// 		DBG("will not stop unstarted listener\n");
// 		return;
// 	}
	
	CRTX_DBG("stopping listener %s\n", listener->id);
	
	LOCK(listener->state_mutex);
	
// 	if (listener->state == CRTX_LSTNR_STOPPING || listener->state == CRTX_LSTNR_STOPPED || listener->state == CRTX_LSTNR_SHUTDOWN) {
	if (listener->state == CRTX_LSTNR_STOPPED || listener->state == CRTX_LSTNR_SHUTDOWN) {
		UNLOCK(listener->state_mutex);
		return EALREADY;
	}
	
	listener->state = CRTX_LSTNR_STOPPING;
	
	LOCK(listener->dependencies_lock);
	if (listener->rev_dependencies) {
		struct crtx_ll *ll;
		struct crtx_listener_base *dep;
		int ready;
		
		ready = 1;
		for (ll = listener->rev_dependencies; ll; ll = ll->next) {
			dep = (struct crtx_listener_base *) ll->data;
			if ( dep->state != CRTX_LSTNR_STOPPED && dep->state != CRTX_LSTNR_SHUTDOWN) {
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
	
	if (listener->evloop_fd.fd >= 0) {
// 		crtx_root->event_loop.del_fd(
// 			&crtx_root->event_loop,
// 			&listener->evloop_fd);
		
		crtx_evloop_disable_cb(listener->evloop_fd.evloop, &listener->default_el_cb);
	}
	
	if (listener->eloop_thread) {
// 		listener->thread->stop = 1;
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
	
	if (listener->state_graph) {
		int ret;
		struct crtx_event *event;
		
		ret = crtx_create_event(&event);
		if (ret < 0) {
			ERROR("crtx_create_event failed: %s\n", strerror(ret));
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
static void free_listener_intern(struct crtx_listener_base *listener) {
// 	crtx_stop_listener(listener);
	
// 	LOCK(listener->state_mutex);
// 	listener->state = CRTX_LSTNR_SHUTDOWN;
// 	UNLOCK(listener->state_mutex);
	
	if (listener->graph && listener->graph->listener == listener)
		crtx_free_graph(listener->graph);
	
// 	if (listener->eloop_thread) {
// 		crtx_wait_on_signal(&listener->eloop_thread->finished);
// 		crtx_dereference_signal(&listener->eloop_thread->finished);
// 		listener->eloop_thread = 0;
// 	}
	
	if (listener->free_cb) {
		listener->free_cb(listener, listener->free_cb_userdata);
	}
}

void crtx_free_listener(struct crtx_listener_base *listener) {
// 	struct crtx_ll *ll;
// 	
// 	
// 	LOCK(listener->rev_dependencies_lock);
// 	for (ll=listener->rev_dependencies; ll; ll=ll->next) {
// 		
// 	}
// 	UNLOCK(listener->rev_dependencies_lock);
	
	crtx_stop_listener(listener);
	
	LOCK(listener->state_mutex);
	
	if (listener->state < CRTX_LSTNR_STOPPED) {
		DBG("listener is not stopped\n");
		UNLOCK(listener->state_mutex);
		return;
	}
	
	if (listener->state == CRTX_LSTNR_SHUTDOWN) {
		UNLOCK(listener->state_mutex);
		return;
	}
	
	CRTX_DBG("shutdown listener %s (%p)\n", listener->id, listener);
	
	listener->state = CRTX_LSTNR_SHUTDOWN;
	
	UNLOCK(listener->state_mutex);
	
	if (listener->evloop_fd.fd >= 0) {
		struct crtx_evloop_callback *cit, *citn;
		
		for (cit = listener->evloop_fd.callbacks; cit; cit=citn) {
			citn = (struct crtx_evloop_callback *) cit->ll.next;
			
			crtx_evloop_remove_cb(listener->evloop_fd.evloop, cit);
			crtx_ll_unlink((struct crtx_ll**) &listener->evloop_fd.callbacks, &cit->ll);
		}
	}
	
	if (listener->shutdown) {
		listener->shutdown(listener);
	} else {
		if (listener->evloop_fd.fd >= 0) {
			close(listener->evloop_fd.fd);
			listener->evloop_fd.fd = 0;
		}
	}
	
	LOCK(crtx_root->listeners_mutex);
	crtx_ll_unlink(&crtx_root->listeners, &listener->ll);
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
			DBG("delaying listener shutdown due to pending events\n");
			UNLOCK(listener->graph->queue_mutex);
			UNLOCK(listener->state_mutex);
			return;
		}
		UNLOCK(listener->graph->queue_mutex);
	}
	
	free_listener_intern(listener);
	
// 	UNLOCK(listener->state_mutex);
}

void traverse_graph_r(struct crtx_graph *graph, struct crtx_task *ti, struct crtx_event *event) {
	void *sessiondata[3];
	char keep_going=1;
	
	if (ti->handle && (!ti->event_type_match || !strcmp(ti->event_type_match, event->description))) {
		DBG("execute task %s with event %d \"%s\" (%p)\n", ti->id, event->type, event->description, event);
		
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
		DBG("execute task %s with event %d \"%s\" (%p) cleanup\n", ti->id, event->type, event->description, event);
		
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
// 	for (; eit; eit = eit->next) {printf("ev %p\n", eit->data);}
// 	eit = graph->equeue;
	for (; eit && (eit->event->claimed || eit->event->error); eit = eit->next) {}
	if (eit && !eit->event->claimed && !eit->event->error) {
		eit->event->claimed = 1;
	}
	UNLOCK(graph->queue_mutex);
	
	if (event)
		*event = eit;
}

// void crtx_claim_next_event(struct crtx_dll **graph, struct crtx_dll **event) {
// 	struct crtx_dll *git;
// 	
// 	if (graph && *graph)
// 		git = *graph;
// 	else
// 		git = crtx_root->graph_queue;
// 	
// 	LOCK(crtx_root->graph_queue_mutex);
// 	for (; git; git = git->next) {
// 		crtx_claim_next_event_of_graph(git->graph, event);
// 		if (*event)
// 			break;
// 	}
// 	UNLOCK(crtx_root->graph_queue_mutex);
// 	
// 	if (graph)
// 		*graph = git;
// }


void crtx_process_event(struct crtx_graph *graph, struct crtx_dll *queue_entry) {
	if (graph->tasks) {
		traverse_graph_r(graph, graph->tasks, queue_entry->event);
	} else {
		if (graph->descriptions && graph->n_descriptions > 0)
			INFO("no task in graph type[0] = %s\n", graph->descriptions[0]);
		else
		if (graph->types && graph->types > 0)	
			INFO("no task in graph %d\n", graph->types[0]);
		else
			INFO("no task in graph\n");
	}
	
	dereference_event_response(queue_entry->event);
	
	dereference_event_release(queue_entry->event);
	
	LOCK(graph->queue_mutex);
	if (queue_entry->prev)
		queue_entry->prev->next = queue_entry->next;
	else
		graph->equeue = queue_entry->next;
	if (queue_entry->next)
		queue_entry->next->prev = queue_entry->prev;
	
// 	if (!graph->equeue) {
// 		LOCK(crtx_root->graph_queue_mutex);
// 		crtx_dll_unlink(&crtx_root->graph_queue, &graph->ll_hdr);
// 		UNLOCK(crtx_root->graph_queue_mutex);
// 	}
	
	if (!graph->equeue)
		pthread_cond_signal(&graph->queue_cond);
	
	UNLOCK(graph->queue_mutex);
	
	free(queue_entry);
	
	if (!graph->equeue && graph->listener && graph->listener->state == CRTX_LSTNR_SHUTDOWN) {
		free_listener_intern(graph->listener);
	}
}

void *crtx_process_graph_tmain(void *arg) {
	// 	struct crtx_event_ll *qe;
	struct crtx_dll *qe;
	struct crtx_graph *graph = (struct crtx_graph*) arg;
	
// 	LOCK(graph->queue_mutex);
// 	
// 	while (!graph->stop) {
// 		for (qe = graph->equeue; qe && (qe->event->claimed || qe->event->error); qe = qe->next) {}
// 		if (qe && !qe->event->claimed && !qe->event->error) {
// 			qe->event->claimed = 1;
// 			break;
// 		}
// 		pthread_cond_wait(&graph->queue_cond, &graph->queue_mutex);
// 	}
// 	
// 	UNLOCK(graph->queue_mutex);
	
// 	while (1) {
		qe = 0;
		crtx_claim_next_event_of_graph(graph, &qe);
		if (!qe)
// 			break;
			return 0;
		
		crtx_process_event(graph, qe);
// 	}
	
	return 0;
}

void add_task(struct crtx_graph *graph, struct crtx_task *task) {
	struct crtx_task *ti, *last;
	
	task->graph = graph;
	
	if (!graph->tasks) {
		graph->tasks = task;
		task->prev = 0;
		task->next = 0;
		return;
	}
	
	last = 0;
	for (ti=graph->tasks;ti;ti=ti->next) {
		if (task->position < ti->position) {
			if (last) {
				task->next = last->next;
				last->next = task;
				task->prev = last;
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
	add_task(graph, task);
	
	return task;
}

// void event_ll_add(struct crtx_event_ll **list, struct crtx_event *event) {
// 	struct crtx_event_ll *eit;
// 	
// 	for (eit=*list; eit && eit->next; eit = eit->next) {}
// 	
// 	if (eit) {
// 		eit->next = (struct crtx_event_ll*) malloc(sizeof(struct crtx_event_ll));
// 		eit->next->prev = eit;
// 		eit = eit->next;
// 	} else {
// 		*list = (struct crtx_event_ll*) malloc(sizeof(struct crtx_event_ll));
// 		eit = *list;
// 		eit->prev = 0;
// 	}
// 	eit->event = event;
// 	eit->in_process = 0;
// 	eit->next = 0;
// }

char is_graph_empty(struct crtx_graph *graph, char *event_type) {
	return (graph->tasks == 0);
}

// void graph_on_thread_finish(struct crtx_thread *thread, void *data) {
// 	struct crtx_graph *graph = (struct crtx_graph*) data;
// 	
// 	ATOMIC_FETCH_SUB(graph->n_consumers, 1);
// }

void graph_consumer_stop(struct crtx_thread *t, void *data) {
	struct crtx_graph *graph = (struct crtx_graph *) data;
	
	graph->stop = 1;
	pthread_cond_broadcast(&graph->queue_cond);
}

void crtx_wait_on_graph_empty(struct crtx_graph *graph) {
	LOCK(graph->queue_mutex);
	
	while (graph->equeue)
		pthread_cond_wait(&graph->queue_cond, &graph->queue_mutex);
	
	UNLOCK(graph->queue_mutex);
}

void crtx_add_event(struct crtx_graph *graph, struct crtx_event *event) {
// 	unsigned int new_n_consumers;
	
	// we do not accept new events if shutdown has begun
// 	if (crtx_root->shutdown) {
// 		DBG("ignoring new events (%s) after shutdown signal\n", event->description);
// 		free_event(event);
// 		return;
// 	}
	
	reference_event_release(event);
	reference_event_response(event);
	
	pthread_mutex_lock(&graph->mutex);
	
	if (!graph->tasks) {
		INFO("dropping event %s (%p) as graph \"%s\" (%p) is empty\n", event->description, event, graph->name?graph->name:"", graph);
		
		pthread_mutex_unlock(&graph->mutex);
		return;
	}
	
	DBG("adding event %s (%p) to graph \"%s\" (%p)\n", event->description, event, graph->name?graph->name:"", graph);
	
	LOCK(graph->queue_mutex);
	
// 	if (!graph->equeue) {
// 		LOCK(crtx_root->graph_queue_mutex);
// 		crtx_dll_append(&crtx_root->graph_queue, &graph->ll_hdr);
// 		UNLOCK(crtx_root->graph_queue_mutex);
// 	}
	
	event->origin = graph->listener;
	
// 	event_ll_add(&graph->equeue, event);
	crtx_dll_append_new(&graph->equeue, event);
	
// 	graph->n_queue_entries++;
	
	pthread_cond_signal(&graph->queue_cond);
	
	UNLOCK(graph->queue_mutex);
	
// // 	new_n_consumers = ATOMIC_FETCH_ADD(graph->n_consumers, 1);
// // 	if (!graph->n_max_consumers || new_n_consumers < graph->n_max_consumers) {
// 	if (! crtx_root->no_threads) {
// 		struct crtx_thread *t;
// 		
// 		t = get_thread(&graph_consumer_main, graph, 0);
// 		if (t) {
// 			t->on_finish = &graph_on_thread_finish;
// 			t->on_finish_data = graph;
// 			t->do_stop = &graph_consumer_stop;
// 			
// 			start_thread(t);
// 		} else {
// // 			ATOMIC_FETCH_SUB(graph->n_consumers, 1);
// 		}
// // 	} else {
// // 		ATOMIC_FETCH_SUB(graph->n_consumers, 1);
// // 	}
	
	enum crtx_processing_mode mode = crtx_get_mode(graph->mode);
	
// 	if (mode == CRTX_PREFER_THREAD || !crtx_root->event_loop.listener) {
	if (mode == CRTX_PREFER_THREAD) {
		struct crtx_thread *t;
		
// 		t = get_thread(&crtx_process_graph_tmain, graph, 0);
		graph->thread_job.fct = &crtx_process_graph_tmain;
		graph->thread_job.fct_data = graph;
		graph->thread_job.do_stop = &graph_consumer_stop;
		t = crtx_thread_assign_job(&graph->thread_job);
// 		if (t) {
// 			t->on_finish = &graph_on_thread_finish;
// 			t->on_finish_data = graph;
// 			t->do_stop = &graph_consumer_stop;
			
// 			start_thread(t);
			crtx_thread_start_job(t);
// 		}
	} else {
// 		if (!crtx_root->event_loop.listener)
// 			crtx_get_event_loop();
// 		crtx_epoll_queue_graph(crtx_root->event_loop.listener, graph);
		
// 		crtx_get_main_event_loop();
		
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
	
	ret = pthread_mutex_init(&(*event)->mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&(*event)->response_cond, NULL); ASSERT(ret >= 0);
	ret = pthread_cond_init(&(*event)->release_cond, NULL); ASSERT(ret >= 0);
	
	return 0;
}

void reference_event_release(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	event->refs_before_release++;
	VDBG("ref release of event %s (%p) (now %d)\n", event->description, event, event->refs_before_release);
	
	pthread_mutex_unlock(&event->mutex);
}

void dereference_event_release(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	VDBG("deref release of event %s (%p) (remaining %d)\n", event->description, event, event->refs_before_release);
	
	if (event->refs_before_release > 0)
		event->refs_before_release--;
	
	if (event->refs_before_release == 0) {
		pthread_mutex_unlock(&event->mutex);
		free_event(event);
		return;
	}
	
	pthread_mutex_unlock(&event->mutex);
}

void reference_event_response(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	if (!event->error) {
		event->refs_before_response++;
		VDBG("ref response of event %s (%p) (now %d)\n", event->description, event, event->refs_before_response);
	}
	
	pthread_mutex_unlock(&event->mutex);
}

void dereference_event_response(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	VDBG("ref response of event %s (%p) (remaining %d)\n", event->description, event, event->refs_before_response);
	
	if (event->refs_before_response > 0)
		event->refs_before_response--;
	
	if (event->refs_before_response == 0)
		pthread_cond_broadcast(&event->response_cond);
	
	pthread_mutex_unlock(&event->mutex);
}

char wait_on_event(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	while (event->refs_before_response > 0)
		pthread_cond_wait(&event->response_cond, &event->mutex);
	
	pthread_mutex_unlock(&event->mutex);
	
	return event->error;
}

// struct crtx_event *new_event() {
// 	struct crtx_event *event;
// 	int ret;
// 	
// 	event = (struct crtx_event*) calloc(1, sizeof(struct crtx_event));
// 	
// 	ret = pthread_mutex_init(&event->mutex, 0); ASSERT(ret >= 0);
// 	ret = pthread_cond_init(&event->response_cond, NULL); ASSERT(ret >= 0);
// 	ret = pthread_cond_init(&event->release_cond, NULL); ASSERT(ret >= 0);
// 	
// 	return event;
// }

// void free_event_data(struct crtx_event_data *ed) {
// // 	if (ed->raw.pointer && !(ed->flags & CRTX_EVF_DONT_FREE_RAW))
// // 		free(ed->raw);
// 	crtx_free_dict_item(&ed->raw);
// 	if (ed->dict)
// 		crtx_free_dict(ed->dict);
// }

void crtx_invalidate_event(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	event->error = 1;
	event->refs_before_response = 0;
	pthread_mutex_unlock(&event->mutex);
	
	pthread_cond_broadcast(&event->response_cond);
}

void free_event(struct crtx_event *event) {
	// is somebody already releasing this event?
	if (!CAS(&event->release_in_progress, 0, 1)) {
		pthread_cond_broadcast(&event->release_cond);
		return;
	}
	
// 	if (event->refs_before_release > 0)
// 		reference_event_release(event);
	
	// disappoint everyone who still waits for this event
	crtx_invalidate_event(event);
	
	// wait until all references are gone
	pthread_mutex_lock(&event->mutex);
	while (event->refs_before_release > 0)
		pthread_cond_wait(&event->release_cond, &event->mutex);
	pthread_mutex_unlock(&event->mutex);
	
	
	if (event->cb_before_release)
		event->cb_before_release(event, event->cb_before_release_data);
	
// 	free_event_data(&event->data);
// 	free_event_data(&event->response);
	crtx_free_dict_item(&event->data);
	crtx_free_dict_item(&event->response);
	
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

struct crtx_graph *find_graph_by_name(char *name) {
	unsigned int i;
	
	for (i=0; i < crtx_root->n_graphs; i++) {
		if (!strcmp(crtx_root->graphs[i]->name, name)) {
			return crtx_root->graphs[i];
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

void add_raw_event(struct crtx_event *event) {
	struct crtx_graph *graph;
	
	graph = 0;
	
	if (event->description)
		graph = crtx_find_graph_for_event_description(event->description);
	
	if (!graph)
		graph = crtx_find_graph_for_event_type(event->type);
	
// 	if (!graph && !strcmp(event->type, CRTX_EVT_NOTIFICATION)) {
// 		crtx_init_notification_listeners(&crtx_root->notification_listeners_handle);
// 		
// 		graph = find_graph_for_event_type(event->type);
// 	}
	
	if (!graph) {
		ERROR("did not find graph for event type \"%s\"\n", event->description);
		return;
	}
	
	crtx_add_event(graph, event);
}


int crtx_init_graph(struct crtx_graph *graph, const char *name) {
	int i, ret;
	
	ret = pthread_mutex_init(&graph->mutex, 0); ASSERT(ret >= 0);
	
	ret = pthread_mutex_init(&graph->queue_mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&graph->queue_cond, NULL); ASSERT(ret >= 0);
	
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
	
	graph = (struct crtx_graph*) calloc(1, sizeof(struct crtx_graph)); ASSERT(graph);
	
	if (crtx_graph)
		*crtx_graph = graph;
	
	return crtx_init_graph(graph, name);
	
// 	, char **event_descriptions
// 	if (event_descriptions) {
// 		while (event_descriptions[graph->n_types]) {
// 			graph->n_types++;
// 		}
// 		
// 		graph->descriptions = event_descriptions;
// 	}
}

static int shutdown_graph_intern(struct crtx_graph *egraph, char crtx_shutdown) {
	size_t i;
	struct crtx_task *t, *tnext;
	struct crtx_dll *qe, *qe_next;
	
	DBG("free graph %s t[0]: %d\n", egraph->name, egraph->types?egraph->types[0]:0);
	
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
		free_event(qe->event);
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
	
	DBG("free task %s\n", task->id);
	
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

void free_task(struct crtx_task *task) {
	crtx_free_task(task);
}

void crtx_flush_events() {
	size_t i;
	
	for (i=0; i<crtx_root->n_graphs; i++) {
		struct crtx_dll *qe, *qe_next;
		
		if (!crtx_root->graphs[i])
			continue;
		
		LOCK(crtx_root->graphs[i]->queue_mutex);
// 		printf("flush %s\n", graphs[i]->types? graphs[i]->types[0]: 0);
		for (qe = crtx_root->graphs[i]->equeue; qe ; qe = qe_next) {
			qe_next = qe->next;
// 			graphs[i]->n_queue_entries--;
			
			crtx_invalidate_event(qe->event);
			
// 			free(qe);
		}
// 		graphs[i]->equeue = 0;
		
		UNLOCK(crtx_root->graphs[i]->queue_mutex);
	}
}

// static char shutdown_event_callback(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct crtx_ll *ll, *ll_next;
// // 	int all_stopped, r;
// 	
// 	VDBG("releasing listeners\n");
// 	
// // 	all_stopped = 1;
// 	for (ll=crtx_root->listeners; ll; ll=ll_next) {
// 		ll_next = ll->next;
// 		
// 		if (crtx_root->event_loop->listener == ll->data)
// 			continue;
// 		
// 		crtx_free_listener((struct crtx_listener_base *) ll->data);
// 		
// // 		all_stopped = 0;
// 		
// // 		r = crtx_stop_listener((struct crtx_listener_base *) ll->data);
// // 		if (r != EALREADY)
// // 			all_stopped = 0;
// 	}
// 	
// // 	printf("all stopped %d\n", all_stopped);
// // 	if (all_stopped) {
// 		if (crtx_root->event_loop->listener) {
// 			crtx_evloop_stop(crtx_root->event_loop);
// 		}
// // 	}
// 	
// 	return 0;
// }

void crtx_init_shutdown() {
	if (crtx_root->shutdown)
		return;
	
	VDBG("init shutdown\n");
	
	crtx_root->shutdown = 1;
	
// 	for (ll=crtx_root->listeners; ll; ll=ll->next) {
// 	while (crtx_root->listeners) {
		struct crtx_ll *ll, *ll_next;
// 		int all_stopped; //, r;
		
// 		all_stopped = 1;
		for (ll=crtx_root->listeners; ll; ll=ll_next) {
			ll_next = ll->next;
			
			if (crtx_root->event_loop && crtx_root->event_loop->listener == ll->data) {
				VDBG("skip eloop listener\n");
				continue;
			}
			
// 			crtx_free_listener((struct crtx_listener_base *) ll->data);
			crtx_stop_listener((struct crtx_listener_base *) ll->data);
// 			r = crtx_stop_listener((struct crtx_listener_base *) ll->data);
// 			if (r != EALREADY)
// 				all_stopped = 0;
		}
	
// 	crtx_root->event_loop->phase_out = 1;
	
	// just wake-up the event loop
// 	crtx_evloop_trigger_callback(crtx_get_main_event_loop(), 0);
	
// 		if (all_stopped)
// 			break;
// 	}
	
// 	crtx_root->shutdown_el_cb.event_handler = &shutdown_event_callback;
// 	crtx_root->shutdown_el_cb.crtx_event_flags = EVLOOP_SPECIAL;
// 	crtx_evloop_trigger_callback(crtx_root->event_loop, &crtx_root->shutdown_el_cb);
	
// 	VDBG("asd %p %d\n",&crtx_root->shutdown_el_cb, crtx_root->shutdown_el_cb.crtx_event_flags);
// 	if (crtx_root->event_loop.listener) {
// // 		crtx_epoll_stop(crtx_root->event_loop.listener);
// 		crtx_evloop_stop(&crtx_root->event_loop);
// 	}
	
// // 	crtx_finish();
// 	crtx_threads_stop_all();
// 	crtx_flush_events();
}

void crtx_shutdown_after_fork() {
	// after a fork, we close all file handles but we don't want to remove
	// them from an epoll instance as it would remove the file handle for
	// the parent process as well.
	//
	// this also causes the child eventloop to stop processing further events
	
// 	crtx_root->after_fork_close = 1;
	crtx_root->event_loop->after_fork_close = 1;
	
	// we do not process the events in the queue
	crtx_flush_events();
	
	crtx_evloop_stop(crtx_root->event_loop);
	
	// create a new event loop that handles shutdown
	crtx_root->event_loop = 0;
	crtx_get_main_event_loop();
	
	crtx_init_shutdown();
}

static char handle_shutdown(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_task *t;
	
	// TODO make sure we're only called once
	t = (struct crtx_task *) userdata;
	t->handle = 0;
	
	crtx_init_shutdown();
	
	if (crtx_root->event_loop && crtx_root->event_loop->listener) {
		crtx_evloop_stop(crtx_root->event_loop);
	}
	
	return 1;
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
	
	DBG("load plugin \"%s\"\n", path);
	
	crtx_root->n_plugins++;
	crtx_root->plugins = (struct crtx_lstnr_plugin*) realloc(crtx_root->plugins, sizeof(struct crtx_lstnr_plugin)*crtx_root->n_plugins);
	p = &crtx_root->plugins[crtx_root->n_plugins-1];
	
	memset(p, 0, sizeof(struct crtx_lstnr_plugin));
	p->path = path;
	p->basename = basename;
	p->handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
	
	if (!p->handle)
		return;
	
	len = strlen(basename);
	if (len <= 12)
		return;
	
	memcpy(plugin_name, basename+8, len - 8 - 3);
	plugin_name[len - 8 - 3] = 0;
	
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
// 					crtx_root->listener_repository_length++;
// 					crtx_root->listener_repository = (struct crtx_listener_repository*) realloc(crtx_root->listener_repository, sizeof(struct crtx_listener_repository)*(crtx_root->listener_repository_length+1));
// 					lrepo = &crtx_root->listener_repository[crtx_root->listener_repository_length-1];
// 					memset(&crtx_root->listener_repository[crtx_root->listener_repository_length], 0, sizeof(struct crtx_listener_repository));
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
	
	if (!directory)
		return;
	
	dhandle = opendir(directory);
	if (dhandle != NULL) {
		while ((dent = readdir(dhandle)) != NULL) {
			if (!strncmp(dent->d_name, "libcrtx_", 8)) {
				char * fullname = (char*) malloc(strlen(directory)+strlen(dent->d_name)+2);
				sprintf(fullname, "%s/%s", directory, dent->d_name);
				
				load_plugin(fullname, dent->d_name);
			}
		}
		closedir(dhandle);
	} else {
		printf("cannot open plugin directory \"%s\"\n", directory);
	}
	
	return;
}

#include <sys/types.h>
#include <sys/stat.h>
void crtx_daemonize() {
// 	pid_t pid = 0;
// 	int r, fd;
	
// 	signal(SIGCHLD, SIG_IGN);
	
	daemon(0, 0);
	
// 	pid = fork();
// 	if (pid < 0) {
// 		ERROR("fork failed\n");
// 		exit(1);
// 	}
// 	if (pid > 0) {
// 		DBG("parent exits\n");
// 		exit(0);
// 	}
// 	
// 	r = setsid();
// 	if (r < 0) {
// 		ERROR("setsid failed: %d\n", r);
// 		exit(1);
// 	}
// 	
// 	umask(0);
// 	chdir("/");
	
// 	for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--) {
// 		close(fd);
// 	}
// 	fclose(stdin);
	
// 	stdin = fopen("/dev/null", "r");
// 	stdout = fopen("/dev/null", "w+");
// 	stderr = fopen("/dev/null", "w+");
}

int crtx_init() {
	struct crtx_task *t;
	unsigned int i;
	
// 	if (!root_ptr) {
// 		root = &global_root;
// 	} else {
// 		*root_ptr = (struct crtx_root*) malloc(sizeof(struct crtx_root));
// 		root = *root_ptr;
// 	}
	
	if (getenv("CRTX_VERBOSITY"))
		crtx_verbosity = atoi(getenv("CRTX_VERBOSITY"));
	
// 	memset(crtx_root, 0, sizeof(struct crtx_root));
	
	DBG("initialized cortex (PID: %d)\n", getpid());
	
	crtx_root->shutdown = 0;
// 	crtx_root->no_threads = 1;
	crtx_root->default_mode = CRTX_PREFER_ELOOP;
	crtx_root->global_fd_flags = O_CLOEXEC;
	
	if (crtx_root->event_loop)
		crtx_root->event_loop->after_fork_close = 0;
	crtx_root->reinit_after_shutdown = 0;
	
	INIT_MUTEX(crtx_root->graphs_mutex);
	
	crtx_create_graph(&crtx_root->crtx_ctrl_graph, "cortexd.control");
	{
		while (crtx_evt_control[crtx_root->crtx_ctrl_graph->n_types]) {
			crtx_root->crtx_ctrl_graph->n_types++;
		}
		
		crtx_root->crtx_ctrl_graph->descriptions = crtx_evt_control;
	}
	
	t = new_task();
	t->id = "shutdown";
	t->handle = &handle_shutdown;
	t->userdata = t;
	t->position = 255;
	t->event_type_match = CRTX_EVT_SHUTDOWN;
	add_task(crtx_root->crtx_ctrl_graph, t);
	
	crtx_threads_init();
	
	i=0;
	while (static_modules[i].id) {
		DBG("initialize \"%s\"\n", static_modules[i].id);
		
		static_modules[i].init();
		i++;
	}
	
	load_plugins(CRTX_PLUGIN_DIR);
	
	return 0;
}

int crtx_finish() {
	unsigned int i;
// 	struct crtx_ll *it, *itn;
	
	crtx_init_shutdown();
	
	VDBG("crtx_finish()\n");
	
	crtx_threads_stop_all();
	crtx_flush_events();
	
	while (crtx_root->listeners) {
		crtx_free_listener((struct crtx_listener_base*) crtx_root->listeners->data);
	}
	
// 	if (crtx_root->event_loop) {
// 		crtx_evloop_stop(crtx_root->event_loop);
// 		crtx_evloop_release(crtx_root->event_loop);
// 	}
// 	for (it=crtx_root->event_loops; it; it=itn) {
// 		itn = it->next;
// 		
// 		crtx_ll_unlink(&crtx_root->event_loops, it);
// 		
// 		crtx_evloop_stop((struct crtx_event_loop*) it->data);
// 		crtx_evloop_release((struct crtx_event_loop*) it->data);
// 		
// 		free(it);
// 	}
	while (crtx_root->event_loops) {
		crtx_evloop_release((struct crtx_event_loop*) crtx_root->event_loops);
	}
	crtx_root->event_loop = 0;
	
	i=0;
	while (static_modules[i].id) { i++; }
	i--;
	
// 	if (crtx_root->notification_listeners_handle)
// 		crtx_finish_notification_listeners(crtx_root->notification_listeners_handle);
	
	// stop threads first
// 	static_modules[0].finish();
	crtx_threads_stop_all();
	// stop controls second
// 	static_modules[1].finish();
	
	while (i > 0) {
		DBG("finish \"%s\"\n", static_modules[i].id);
		
		static_modules[i].finish();
		i--;
	}
	
// 	if (crtx_root->event_loop && crtx_root->event_loop->listener) {
// 		crtx_free_listener((struct crtx_listener_base *) crtx_root->event_loop->listener);
// // 		free(crtx_root->event_loop.listener);
// 	}
	
// 	if (crtx_root->event_loop)
// 		free(crtx_root->event_loop);
// 	crtx_root->event_loop = 0;
	
	// finish threads module
// 	static_modules[0].finish();
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
	
	DBG("finished shutdown\n");
	
	return 0;
}

void print_tasks(struct crtx_graph *graph) {
	struct crtx_task *e;
	
	for (e=graph->tasks; e; e=e->next) {
		INFO("%u %s %s\n", e->position, e->id, e->event_type_match);
	}
}

void hexdump(unsigned char *buffer, size_t index) {
	size_t i;
	
	for (i=0;i<index;i++) {
		INFO("%02x ", buffer[i]);
	}
	INFO("\n");
}

typedef char (*event_notifier_filter)(struct crtx_event *event);
typedef void (*event_notifier_cb)(struct crtx_event *event);
struct crtx_event_notifier {
	event_notifier_filter filter;
	event_notifier_cb callback;
	
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

void event_notifier_task(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_notifier *en = (struct crtx_event_notifier*) userdata;
	
	if (en->filter(event)) {
		en->callback(event);
		
		pthread_mutex_lock(&en->mutex);
		pthread_cond_broadcast(&en->cond);
		pthread_mutex_unlock(&en->mutex);
	}
}

void wait_on_notifier(struct crtx_event_notifier *en) {
	pthread_mutex_lock(&en->mutex);
	
	pthread_cond_wait(&en->cond, &en->mutex);
	
	pthread_mutex_unlock(&en->mutex);
}

char *get_username() {
	struct passwd *pw;
	uid_t uid;
	
	uid = getuid();
	pw = getpwuid(uid);
	if (pw) {
		return pw->pw_name;
	}
	
	return 0;
}

// void *crtx_copy_raw_data(struct crtx_event_data *data) {
// 	void *new_data;
// 	
// 	new_data = malloc(data->raw.size);
// 	memcpy(new_data, data->raw.pointer, data->raw.size);
// 	
// 	return new_data;
// }

// void crtx_fprintf(int fd, char *fmt, ...) {
	// check if last char is \n
// }

void crtx_loop() {
	if (!crtx_root->event_loop)
		return;
	
	// the thread calling this either executes the event loop or becomes one of
	// the worker threads in the thread pool
	if (crtx_root->event_loop->listener) {
// 		crtx_epoll_main(crtx_root->event_loop.listener);
		crtx_evloop_start(crtx_root->event_loop);
	} else {
		spawn_thread(0);
	}
	
	DBG("crtx_loop() end\n");
	
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

void crtx_loop_onetime() {
	crtx_root->event_loop->onetime(crtx_root->event_loop);
}

// static void *event_loop_tmain(void *data) {
// 	crtx_epoll_main(crtx_root->event_loop.listener);
// 	
// 	return 0;
// }
// 
// struct crtx_thread * crtx_start_detached_event_loop() {
// 	if (!crtx_root->event_loop.listener)
// 		crtx_get_event_loop();
// 	
// 	if (!crtx_root->event_loop.listener) {
// 		ERROR("no event loop registered\n");
// 		return 0;
// 	}
// 	
// 	return get_thread(event_loop_tmain, 0, 1);
// }

// void crtx_init_notification_listeners(void **data) {
// #ifdef TODO
// 	void *result, *tmp;
// 	struct crtx_sd_bus_notification_listener *notify_listener;
// 	struct crtx_readline_listener *rl_listener;
// 	
// 	*data = calloc(1,
// 			sizeof(struct crtx_sd_bus_notification_listener) +
// 			sizeof(struct crtx_readline_listener));
// 	tmp = *data;
// 	
// 	notify_listener = (struct crtx_sd_bus_notification_listener *) tmp;
// 	tmp += sizeof(struct crtx_sd_bus_notification_listener);
// 	
// 	// create a listener (queue) for notification events
// 	result = create_listener("sd_bus_notification", notify_listener);
// 	if (!result) {
// 		printf("cannot create fanotify listener\n");
// 	}
// 	
// 	rl_listener = (struct crtx_readline_listener *) tmp;
// 	tmp += sizeof(struct crtx_readline_listener);
// 	
// 	// create a listener (queue) for notification events
// 	result = create_listener("readline", &rl_listener);
// 	if (!result) {
// 		printf("cannot create fanotify listener\n");
// 	}
// #endif
// }
// 
// void crtx_finish_notification_listeners(void *data) {
// #ifdef TODO
// 	struct crtx_listener_base *base;
// 	void *tmp;
// 	
// 	tmp = data;
// 	
// 	base = (struct crtx_listener_base*) tmp;
// 	crtx_free_listener(base);
// 	tmp += sizeof(struct crtx_sd_bus_notification_listener);
// 	
// 	base = (struct crtx_listener_base*) tmp;
// 	crtx_free_listener(base);
// 	tmp += sizeof(struct crtx_readline_listener);
// 	
// 	free(data);
// #endif
// }

// void crtx_event_set_data(struct crtx_event *event, void *raw_pointer, unsigned char flags, struct crtx_dict *data_dict, unsigned char n_additional_fields) {
// 	struct crtx_dict *upgraded_dict;
// 	struct crtx_dict_item *di;
// 	
// 	
// 	LOCK(event->mutex);
// 	
// 	if (raw_pointer || event->data.type == 0) {
// 		switch (event->data.type) {
// 			case 0:
// 			case 'p':
// 				event->data.key = "raw";
// 				event->data.type = 'p';
// 				event->data.size = 0;
// 				event->data.flags = 0;
// 				event->data.pointer = raw_pointer;
// 				break;
// 			case 'D':
// 				di = crtx_get_item_by_idx(event->data.dict, 0);
// 				di->pointer = raw_pointer;
// 				break;
// 			default:
// 				ERROR("invalid event data type %c (%d)\n", event->data.type, event->data.type);
// 				break;
// 		}
// 	}
// 	
// 	if (data_dict || n_additional_fields > 0) {
// 		if (event->data.type == 'p') {
// 			upgraded_dict = crtx_init_dict(0, 2+n_additional_fields, 0);
// 			
// 			di = crtx_get_item_by_idx(upgraded_dict, 0);
// 			
// // 			crtx_dict_copy_item(di, &event->data, 0);
// 			memcpy(di, &event->data, sizeof(struct crtx_dict_item));
// 			
// 			di = crtx_get_item_by_idx(upgraded_dict, 1);
// 			di->key = "data";
// 			di->type = 'D';
// 			di->dict = data_dict;
// 			
// 			event->data.key = "data";
// 			event->data.type = 'D';
// 			event->data.size = 0;
// 			event->data.flags = 0;
// 			event->data.dict = upgraded_dict;
// 		} else
// 		if (event->data.type == 'D') {
// 			if (strcmp(event->data.key, "data"))
// 				ERROR("invalid event data structure\n");
// 			
// 			di = crtx_get_item_by_idx(event->data.dict, 1);
// 			di->dict = data_dict;
// 			
// 			if (event->data.dict->signature_length < 2 + n_additional_fields)
// 				crtx_resize_dict(event->data.dict, 2 + n_additional_fields);
// 		}
// 	}
// 	
// 	UNLOCK(event->mutex);
// }

// void crtx_event_set_raw_data(struct crtx_event *event, void *raw_pointer, size_t size, unsigned char flags) {
void crtx_event_set_raw_data(struct crtx_event *event, unsigned char type, ...) {
	struct crtx_dict *upgraded_dict;
	struct crtx_dict_item *di;
	va_list va;
	
	
	LOCK(event->mutex);
	
	va_start(va, type);
// 	
// 	ret = crtx_fill_data_item_va(di, type, va);
// 	
// 	va_end(va);
// 	crtx_fill_data_item(&event->data, type, "crtx_raw_data", raw_pointer, size, flags);
	
	if (event->data.type == 'D') {
		if (event->data.key && !strcmp(event->data.key, "crtx_raw_data")) {
			di = crtx_get_item_by_idx(event->data.dict, 0);
			crtx_fill_data_item_va2(di, type, "crtx_raw_data", &va);
		} else {
			upgraded_dict = crtx_init_dict(0, 2, 0);
			
			di = crtx_get_item_by_idx(upgraded_dict, 0);
			
			crtx_fill_data_item_va2(di, type, "crtx_raw_data", &va);
			
			di = crtx_get_item_by_idx(upgraded_dict, 1);
// 			di->key = event->data.key;
// 			di->type = 'D';
// 			di->size = event->data.size;
// 			di->flags = event->data.flags;
// 			di->dict = &event->data;
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
	
// 	if (raw_pointer || event->data.type == 0) {
// 	switch (event->data.type) {
// 		case 0:
// 		case 'p':
// 			event->data.key = "crtx_raw_data";
// 			event->data.type = 'p';
// 			event->data.size = size;
// 			event->data.flags = flags;
// 			event->data.pointer = raw_pointer;
// 			break;
// 		case 'D':
// 			if (event->data.key && !strcmp(event->data.key, "crtx_raw_data")) {
// 				di = crtx_get_item_by_idx(event->data.dict, 0);
// 				di->pointer = raw_pointer;
// 			} else {
// 				upgraded_dict = crtx_init_dict(0, 2+n_additional_fields, 0);
// 				
// 				di = crtx_get_item_by_idx(upgraded_dict, 0);
// 				
// 	// 			crtx_dict_copy_item(di, &event->data, 0);
// // 				memcpy(di, &event->data, sizeof(struct crtx_dict_item));
// 				di->key = "crtx_raw_data";
// 				di->type = 'p';
// 				di->size = size;
// 				di->flags = flags;
// 				di->pointer = raw_pointer;
// 				
// 				di = crtx_get_item_by_idx(upgraded_dict, 1);
// 				di->key = event->data.key;
// 				di->type = 'D';
// 				di->size = event->data.size;
// 				di->flags = event->data.flags;
// 				di->dict = &event->data;
// 				
// 				event->data.key = "crtx_dict_data";
// 				event->data.type = 'D';
// 				event->data.size = 0;
// 				event->data.flags = 0;
// 				event->data.dict = upgraded_dict;
// 			}
// 			break;
// 		default:
// 			ERROR("invalid event data type %c (%d)\n", event->data.type, event->data.type);
// 			break;
// 	}
	
	UNLOCK(event->mutex);
}

void crtx_event_set_dict_data(struct crtx_event *event, struct crtx_dict *data_dict, unsigned char n_additional_fields) {
	struct crtx_dict *upgraded_dict;
	struct crtx_dict_item *di;
// 	unsigned char n_additional_fields = 0;
// 	crtx_event_set_data(event, 0, 0, data_dict, n_additional_fields);
	
	LOCK(event->mutex);
	
// 	if (data_dict || n_additional_fields > 0) {
		if (event->data.type != 'D') {
			upgraded_dict = crtx_init_dict(0, 2+n_additional_fields, 0);
			
			di = crtx_get_item_by_idx(upgraded_dict, 0);
			
// 			crtx_dict_copy_item(di, &event->data, 0);
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
// 			if (strcmp(event->data.key, "data"))
// 				ERROR("invalid event data structure\n");
			
			if (!strcmp(event->data.key, "crtx_dict_data")) {
				di = crtx_get_item_by_idx(event->data.dict, 1);
				di->dict = data_dict;
				
				if (event->data.dict->signature_length < 2 + n_additional_fields)
					crtx_resize_dict(event->data.dict, 2 + n_additional_fields);
			} else {
				event->data.dict = data_dict;
				if (n_additional_fields > 0)
					ERROR("TODO\n");
			}
		}
// 	}
	
	UNLOCK(event->mutex);
}

char crtx_event_raw2dict(struct crtx_event *event, void *user_data) {
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

// static struct crtx_dict_item *crtx_event_get_data_dict(struct crtx_event *event) {
// 	if (event->data.type != 'D' || strcmp(event->data.key, "crtx_dict_data"))
// 		return &event->data;
// 	
// 	return crtx_get_item_by_idx(event->data.dict, 1);
// }

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
				ERROR("unknown event dict %p\n", event->data.dict);
				*raw_pointer = 0;
			}
		} else {
			ERROR("unknown event data type %d\n", event->data.type);
			*raw_pointer = 0;
		}
	}
	if (dict) {
		if (event->data.type != 'D') {
			int ret;
			
			ret = crtx_event_raw2dict(event, 0);
			if (ret != CRTX_SUCCESS) {
				ERROR("cannot convert raw data\n");
				*dict = 0;
				return;
			}
		} 
// 		else
// 		if (event->data.type != 'D') {
// 			ERROR("unknown event data type %d\n", event->data.type);
// 			*dict = 0;
// 			return;
// 		}
		
		if (!strcmp(event->data.key, "crtx_dict_data")) {
			di = crtx_get_item_by_idx(event->data.dict, 1);
			*dict = di->dict;
		} else {
			*dict = event->data.dict;
		}
	}
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

void crtx_register_handler_for_event_type(char *event_type, char *handler_name, crtx_handle_task_t handler_function, void *handler_data) {
	struct crtx_ll *catit, *entry;
	
	for (catit = crtx_root->handler_categories; catit && strcmp(catit->handler_category->event_type, event_type); catit=catit->next) {}
	
// 	if (!last_catit) {
// 		catit = crtx_ll_append_new(&crtx_root->handler_categories, 0);
// 		catit->handler_category = 0;
// 	}
	
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
	
	VDBG("new handler %s for etype %s\n", handler_name, event_type);
}

void crtx_autofill_graph_with_tasks(struct crtx_graph *graph, const char *event_type) {
	struct crtx_ll *catit, *entry;
	
	for (catit = crtx_root->handler_categories; catit && strcmp(catit->handler_category->event_type, event_type); catit=catit->next) {}
	
	if (catit) {
		for (entry = catit->handler_category->entries; entry; entry=entry->next) {
			VDBG("auto-add %s to %s\n", entry->handler_category_entry->handler_name, graph->name);
			crtx_create_task(graph, 0,
							entry->handler_category_entry->handler_name,
							entry->handler_category_entry->function,
							entry->handler_category_entry->handler_data);
		}
	}
}

void crtx_set_main_event_loop(const char *event_loop) {
	crtx_root->chosen_event_loop = event_loop;
}

void crtx_lock_listener_source(struct crtx_listener_base *lbase) {
	LOCK(lbase->source_lock);
}

void crtx_unlock_listener_source(struct crtx_listener_base *lbase) {
	UNLOCK(lbase->source_lock);
}

int crtx_handle_std_signals() {
	return crtx_signals_handle_std_signals(0);
}

void crtx_trigger_event_processing(struct crtx_listener_base *lstnr) {
	crtx_evloop_trigger_callback(lstnr->evloop_fd.evloop, &lstnr->default_el_cb);
}
