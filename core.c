#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <stddef.h>
#include <inttypes.h>

// for get_username()
#include <unistd.h>
#include <pwd.h>

#include "core.h"
#include "dict.h"

#include "core.h"
#include "socket.h"
#include "socket_raw.h"
#ifdef STATIC_SD_BUS
#include "sd_bus.h"
#endif
#ifdef STATIC_NF_QUEUE
#include "nf_queue.h"
#endif
#include "readline.h"
#include "controls.h"
#include "fanotify.h"
#include "inotify.h"
#include "sd_bus_notifications.h"
#include "threads.h"
#include "signals.h"
#include "timer.h"
#include "netlink_raw.h"
#ifdef STATIC_NETLINK_GE
#include "netlink_ge.h"
#endif
#include "epoll.h"
#include "evdev.h"
#include "udev.h"
#ifdef STATIC_XCB_RANDR
#include "xcb_randr.h"
#endif
#ifdef STATIC_PULSEAUDIO
#include "pulseaudio.h"
#endif
#include "can.h"
#include "avahi.h"
#include "uevents.h"
#include "nl_route.h"
#include "libvirt.h"


struct crtx_root crtx_global_root;
struct crtx_root *crtx_root = &crtx_global_root;

struct crtx_module static_modules[] = {
	{"threads", &crtx_threads_init, &crtx_threads_finish},
	{"controls", &crtx_controls_init, &crtx_controls_finish},
	{"signals", &crtx_signals_init, &crtx_signals_finish},
	{"socket", &crtx_socket_init, &crtx_socket_finish},
	{"socket_raw", &crtx_socket_raw_init, &crtx_socket_raw_finish},
#ifdef STATIC_SD_BUS
	{"sdbus", &crtx_sdbus_init, &crtx_sdbus_finish},
	{"sd_bus_notification", &crtx_sdbus_notification_init, &crtx_sdbus_notification_finish},
#endif
#ifdef STATIC_NF_QUEUE
	{"nf-queue", &crtx_nf_queue_init, &crtx_nf_queue_finish},
#endif
	{"readline", &crtx_readline_init, &crtx_readline_finish},
	{"fanotify", &crtx_fanotify_init, &crtx_fanotify_finish},
	{"inotify", &crtx_inotify_init, &crtx_inotify_finish},
	{"netlink_raw", &crtx_netlink_raw_init, &crtx_netlink_raw_finish},
#ifdef STATIC_NETLINK_GE
	{"netlink_ge", &crtx_netlink_ge_init, &crtx_netlink_ge_finish},
#endif
	{"epoll", &crtx_epoll_init, &crtx_epoll_finish},
	{"evdev", &crtx_evdev_init, &crtx_evdev_finish},
	{"udev", &crtx_udev_init, &crtx_udev_finish},
#ifdef STATIC_XCB_RANDR
	{"xcb_randr", &crtx_xcb_randr_init, &crtx_xcb_randr_finish},
#endif
#ifdef STATIC_PULSEAUDIO
	{"pulseaudio", &crtx_pa_init, &crtx_pa_finish},
#endif
	{"can", &crtx_can_init, &crtx_can_finish},
	{"avahi", &crtx_avahi_init, &crtx_avahi_finish},
	{"uevents", &crtx_uevents_init, &crtx_uevents_finish},
	{"nl_route", &crtx_nl_route_init, &crtx_nl_route_finish},
	{0, 0}
};

struct crtx_listener_repository listener_factory[] = {
#ifdef STATIC_NF_QUEUE
	{"nf_queue", &crtx_new_nf_queue_listener},
#endif
	{"fanotify", &crtx_new_fanotify_listener},
	{"inotify", &crtx_new_inotify_listener},
	{"socket_server", &crtx_new_socket_server_listener},
	{"socket_raw_server", &crtx_new_socket_raw_server_listener},
	{"socket_client", &crtx_new_socket_client_listener},
	{"socket_raw_client", &crtx_new_socket_raw_client_listener},
#ifdef STATIC_SD_BUS
	{"sd_bus_notification", &crtx_new_sd_bus_notification_listener},
#endif
	{"signals", &crtx_new_signals_listener},
	{"readline", &crtx_new_readline_listener},
	{"timer", &crtx_new_timer_listener},
	{"netlink_raw", &crtx_new_netlink_raw_listener},
#ifdef STATIC_NETLINK_GE
	{"genl", &crtx_new_genl_listener},
#endif
	{"epoll", &crtx_new_epoll_listener},
	{"evdev", &crtx_new_evdev_listener},
	{"udev", &crtx_new_udev_listener},
#ifdef STATIC_XCB_RANDR
	{"xcb_randr", &crtx_new_xcb_randr_listener},
#endif
	{"sdbus", &crtx_new_sdbus_listener},
#ifdef STATIC_PULSEAUDIO
	{"pulseaudio", &crtx_new_pa_listener},
#endif
	{"can", &crtx_new_can_listener},
	{"avahi", &crtx_new_avahi_listener},
	{"uevents", &crtx_new_uevents_listener},
	{"nl_route", &crtx_new_nl_route_listener},
	{"libvirt", &crtx_new_libvirt_listener},
	{0, 0}
};

char * crtx_evt_control[] = { CRTX_EVT_MOD_INIT, CRTX_EVT_SHUTDOWN, 0 };
char *crtx_evt_notification[] = { CRTX_EVT_NOTIFICATION, 0 };
char *crtx_evt_inbox[] = { CRTX_EVT_INBOX, 0 };
char *crtx_evt_outbox[] = { CRTX_EVT_OUTBOX, 0 };

char crtx_verbosity = CRTX_INFO;

static void new_eventgraph(struct crtx_graph **crtx_graph, char *name, char **event_types);


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

void crtx_printf_va(char level, char *format, va_list va) {
	if (level == CRTX_ERR) {
		vfprintf(stderr, format, va);
	} else {
		if (level <= crtx_verbosity)
			vfprintf(stdout, format, va);
	}
}

void crtx_printf(char level, char *format, ...) {
	va_list va;
	
	va_start(va, format);
	
	crtx_printf_va(level, format, va);
	
	va_end(va);
}

enum crtx_processing_mode crtx_get_mode(enum crtx_processing_mode local_mode) {
	enum crtx_processing_mode mode;
	
	mode = CRTX_PREFER_NONE;
	if (crtx_root->force_mode == CRTX_PREFER_NONE) {
		if (local_mode == CRTX_PREFER_NONE) {
			mode = crtx_root->default_mode;
		} else {
			mode = local_mode;
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

char crtx_start_listener(struct crtx_listener_base *listener) {
	enum crtx_processing_mode mode;
	struct crtx_event *event;
	int ret;
	
	LOCK(listener->state_mutex);
	
	if (listener->state == CRTX_LSTNR_STARTED) {
		DBG("listener already started\n");
		UNLOCK(listener->state_mutex);
		return 1;
	}
	
	listener->state = CRTX_LSTNR_STARTING;
	
// 	if (listener->state == CRTX_LSTNR_STARTED || listener == CRTX_LSTNR_PAUSED) {
// 		DBG("will not start 
// 	}
// 	
// 	listener->state = CRTX_LSTNR_STARTED;
// 	printf("send started1 %p\n", listener);
	if (listener->start_listener) {
		ret = listener->start_listener(listener);
		if (ret == 0 && listener->state_graph) {
// 		if (ret == 0) {
			listener->state = CRTX_LSTNR_STOPPED;
			
			event = crtx_create_event("listener_state", 0, 0);
			
			event->data.type = 'u';
			event->data.uint32 = CRTX_LSTNR_STOPPED;
			
			add_event(listener->state_graph, event);
			
			UNLOCK(listener->state_mutex);
			
			return 0;
		}
	}
	
	if (!listener->el_payload.fd && !listener->thread) {
		DBG("no method to start listener \"%s\" provided\n", listener->id);
		
// 		UNLOCK(listener->state_mutex);
// 		return -1;
	} else {
		mode = crtx_get_mode(listener->mode);
		
		if (mode == CRTX_PREFER_ELOOP && listener->el_payload.fd <= 0) {
			ERROR("listener mode set to \"event loop\" but not event loop data available\n");
			mode = CRTX_PREFER_THREAD;
		}
		if (mode == CRTX_PREFER_THREAD && !listener->thread) {
			ERROR("listener mode set to \"thread\" but not thread data available\n");
			mode = CRTX_PREFER_ELOOP;
		}
		
		if (mode == CRTX_PREFER_THREAD) {
			start_thread(listener->thread);
		} else 
		if (mode == CRTX_PREFER_ELOOP) {
			if (!crtx_root->event_loop.listener)
				crtx_get_event_loop();
			
			DBG("start listener \"%s\"\n", listener->id);
			crtx_root->event_loop.add_fd(
				&crtx_root->event_loop.listener->parent,
				&listener->el_payload);
		} else {
			ERROR("invalid start listener mode: %d\n", mode);
		}
	}
	
	listener->state = CRTX_LSTNR_STARTED;
	
	if (listener->state_graph) {
		event = crtx_create_event("listener_state", 0, 0);
		event->data.type = 'u';
		event->data.uint32 = CRTX_LSTNR_STARTED;
		add_event(listener->state_graph, event);
	}
	
	UNLOCK(listener->state_mutex);
	
	return 1;
}

char crtx_update_listener(struct crtx_listener_base *listener) {
	if (listener->update_listener) {
		listener->update_listener(listener);
	}
	
	return 1;
}

// static void listener_thread_on_finish(struct crtx_thread *thread, void *userdata) {
// 	free_listener( (struct crtx_listener_base*) userdata);
// }

struct crtx_listener_base *create_listener(char *id, void *options) {
	struct crtx_listener_repository *l;
	struct crtx_listener_base *lbase;
	
	DBG("new listener %s\n", id);
	
	lbase = 0;
	l = listener_factory;
	while (l->id) {
		if (!strcmp(l->id, id)) {
			lbase = l->create(options);
			
			if (!lbase) {
				ERROR("creating listener \"%s\" failed\n", id);
				return 0;
			}
			
			if (lbase->thread)
				reference_signal(&lbase->thread->finished);
			
			break;
		}
		l++;
	}
	
	lbase->id = id;
	INIT_MUTEX(lbase->state_mutex);
	lbase->state = CRTX_LSTNR_STOPPED;
// 	printf("if %s\n", id);
	new_eventgraph(&lbase->graph, 0, 0);
	lbase->graph->listener = lbase;
	
// 	new_eventgraph(&lbase->state_graph, 0, 0);
	
	if (!lbase) {
		ERROR("listener \"%s\" not found\n", id);
	} else {
		if (lbase->el_payload.fd > 0) {
			if (!crtx_root->event_loop.listener)
				crtx_get_event_loop();
		} else
		if (lbase->thread) {
// 			if (lbase->thread->on_finish)
// 				ERROR("thread->on_finish already set\n");
// 			lbase->thread->on_finish = &listener_thread_on_finish;
// 			lbase->thread->on_finish_data = lbase;
		}
	}
	
	return lbase;
}

void crtx_stop_listener(struct crtx_listener_base *listener) {
// 	if (listener->state != CRTX_LSTNR_STARTED && listener != CRTX_LSTNR_PAUSED) {
// 		DBG("will not stop unstarted listener\n");
// 		return;
// 	}
// 	
	LOCK(listener->state_mutex);
	
	if (listener->state == CRTX_LSTNR_STOPPING || listener->state == CRTX_LSTNR_STOPPED) {
		UNLOCK(listener->state_mutex);
		return;
	}
	
	listener->state = CRTX_LSTNR_STOPPING;
	
	if (listener->el_payload.fd > 0) {
// 		printf("del fd %d\n", listener->el_payload.fd);
		crtx_root->event_loop.del_fd(
			&crtx_root->event_loop.listener->parent,
			&listener->el_payload);
	}
	
	if (listener->thread) {
// 		listener->thread->stop = 1;
		crtx_threads_stop(listener->thread);
	}
	
	if (listener->stop_listener) {
		listener->stop_listener(listener);
	}
	
	listener->state = CRTX_LSTNR_STOPPED;
	
	if (listener->state_graph) {
		struct crtx_event *event;
		event = crtx_create_event("listener_state", 0, 0);
		event->data.type = 'u';
		event->data.uint32 = CRTX_LSTNR_STOPPED;
		add_event(listener->state_graph, event);
	}
	
	UNLOCK(listener->state_mutex);
	
	return;
}

static void free_listener_intern(struct crtx_listener_base *listener) {
// 	crtx_stop_listener(listener);
	
// 	LOCK(listener->state_mutex);
// 	listener->state = CRTX_LSTNR_SHUTDOWN;
// 	UNLOCK(listener->state_mutex);
	
	if (listener->graph)
		free_eventgraph(listener->graph);
	
// 	&& crtx_signal_is_active(&listener->thread->start)
	if (listener->thread) {
		wait_on_signal(&listener->thread->finished);
		dereference_signal(&listener->thread->finished);
	}
	
	if (listener->free) {
		listener->free(listener, listener->free_userdata);
	}
}

void crtx_free_listener(struct crtx_listener_base *listener) {
	crtx_stop_listener(listener);
	
	LOCK(listener->state_mutex);
	if (listener->state != CRTX_LSTNR_STOPPED) {
		ERROR("listener is not stopped\n");
	}
	
	listener->state = CRTX_LSTNR_SHUTDOWN;
	
	if (listener->shutdown)
		listener->shutdown(listener);
	
	LOCK(listener->graph->queue_mutex);
	if (listener->graph->equeue) {
		DBG("delaying listener shutdown due to pending events\n");
		UNLOCK(listener->graph->queue_mutex);
		UNLOCK(listener->state_mutex);
		return;
	}
	UNLOCK(listener->graph->queue_mutex);
	
	free_listener_intern(listener);
	
// 	UNLOCK(listener->state_mutex);
}

void traverse_graph_r(struct crtx_graph *graph, struct crtx_task *ti, struct crtx_event *event) {
	void *sessiondata[3];
	char keep_going=1;
	
	if (ti->handle && (!ti->event_type_match || !strcmp(ti->event_type_match, event->type))) {
		DBG("execute task %s with event %s (%p)\n", ti->id, event->type, event);
		
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
		DBG("execute task %s with event %s (%p) cleanup\n", ti->id, event->type, event);
		
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

void crtx_claim_next_event(struct crtx_dll **graph, struct crtx_dll **event) {
	struct crtx_dll *git;
	
	if (graph && *graph)
		git = *graph;
	else
		git = crtx_root->graph_queue;
	
	LOCK(crtx_root->graph_queue_mutex);
	for (; git; git = git->next) {
		crtx_claim_next_event_of_graph(git->graph, event);
		if (*event)
			break;
	}
	UNLOCK(crtx_root->graph_queue_mutex);
	
	if (graph)
		*graph = git;
}


void crtx_process_event(struct crtx_graph *graph, struct crtx_dll *queue_entry) {
	if (graph->tasks)
		traverse_graph_r(graph, graph->tasks, queue_entry->event);
	else
		INFO("no task in graph %s\n", graph->types[0]);
	
	dereference_event_response(queue_entry->event);
	
	dereference_event_release(queue_entry->event);
	
	LOCK(graph->queue_mutex);
	if (queue_entry->prev)
		queue_entry->prev->next = queue_entry->next;
	else
		graph->equeue = queue_entry->next;
	if (queue_entry->next)
		queue_entry->next->prev = queue_entry->prev;
	
	if (!graph->equeue) {
		LOCK(crtx_root->graph_queue_mutex);
		crtx_dll_unlink(&crtx_root->graph_queue, &graph->ll_hdr);
		UNLOCK(crtx_root->graph_queue_mutex);
	}
	
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

struct crtx_task *crtx_create_task(struct crtx_graph *graph, unsigned char position, char *id, crtx_handle_task_t handler, void *userdata) {
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

void graph_on_thread_finish(struct crtx_thread *thread, void *data) {
	struct crtx_graph *graph = (struct crtx_graph*) data;
	
	ATOMIC_FETCH_SUB(graph->n_consumers, 1);
}

void graph_consumer_stop(struct crtx_thread *t, void *data) {
	struct crtx_graph *graph = (struct crtx_graph *) data;
	
	graph->stop = 1;
	pthread_cond_broadcast(&graph->queue_cond);
}

void add_event(struct crtx_graph *graph, struct crtx_event *event) {
// 	unsigned int new_n_consumers;
	
	// we do not accept new events if shutdown has begun
	if (crtx_root->shutdown) {
		DBG("ignoring new events (%s) after shutdown signal\n", event->type);
		return;
	}
	
	reference_event_release(event);
	reference_event_response(event);
	
	pthread_mutex_lock(&graph->mutex);
	
	DBG("add event %s (%p) to graph \"%s\" (%p)\n", event->type, event, graph->name?graph->name:"", graph);
	
	if (!graph->tasks) {
		INFO("dropping event as graph is empty\n");
		
		pthread_mutex_unlock(&graph->mutex);
		return;
	}
	
	LOCK(graph->queue_mutex);
	
	if (!graph->equeue) {
		LOCK(crtx_root->graph_queue_mutex);
		crtx_dll_append(&crtx_root->graph_queue, &graph->ll_hdr);
		UNLOCK(crtx_root->graph_queue_mutex);
	}
	
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
		
		t = get_thread(&crtx_process_graph_tmain, graph, 0);
		if (t) {
			t->on_finish = &graph_on_thread_finish;
			t->on_finish_data = graph;
			t->do_stop = &graph_consumer_stop;
			
			start_thread(t);
		}
	} else {
		if (!crtx_root->event_loop.listener)
			crtx_get_event_loop();
		crtx_epoll_queue_graph(crtx_root->event_loop.listener, graph);
	}
	
	pthread_mutex_unlock(&graph->mutex);
}

struct crtx_event *crtx_create_event(char *type, void *data, size_t data_size) {
	struct crtx_event *event;
	int ret;
	
	event = (struct crtx_event*) calloc(1, sizeof(struct crtx_event));
	
	ret = pthread_mutex_init(&event->mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&event->response_cond, NULL); ASSERT(ret >= 0);
	ret = pthread_cond_init(&event->release_cond, NULL); ASSERT(ret >= 0);
	
	event->type = type;
	
// 	event->data.pointer = data;
// 	event->data.type = 'p';
	crtx_event_set_data(event, data, 0, 0);
	event->data.size = data_size;
	
// 	if (complex_data_layout) {
// 		crtx_dict_upgrade_event_data(event, 0);
// 	}
	
	return event;
}

void reference_event_release(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	event->refs_before_release++;
	VDBG("ref release of event %s (%p) (now %d)\n", event->type, event, event->refs_before_release);
	
	pthread_mutex_unlock(&event->mutex);
}

void dereference_event_release(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	VDBG("deref release of event %s (%p) (remaining %d)\n", event->type, event, event->refs_before_release);
	
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
		VDBG("ref response of event %s (%p) (now %d)\n", event->type, event, event->refs_before_response);
	}
	
	pthread_mutex_unlock(&event->mutex);
}

void dereference_event_response(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	VDBG("ref response of event %s (%p) (remaining %d)\n", event->type, event, event->refs_before_response);
	
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
		event->cb_before_release(event);
	
// 	free_event_data(&event->data);
// 	free_event_data(&event->response);
	crtx_free_dict_item(&event->data);
	crtx_free_dict_item(&event->response);
	
	free(event);
}

struct crtx_graph *find_graph_for_event_type(char *event_type) {
	unsigned int i, j;
	
	for (i=0; i < crtx_root->n_graphs; i++) {
		for (j=0; j < crtx_root->graphs[i]->n_types; j++) {
			if (!strcmp(crtx_root->graphs[i]->types[j], event_type)) {
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

struct crtx_graph *get_graph_for_event_type(char *event_type, char **new_event_types) {
	struct crtx_graph *graph;
	
	graph = find_graph_for_event_type(event_type);
	if (!graph) {
		new_eventgraph(&graph, 0, new_event_types);
	}
	
	return graph;
}

void add_raw_event(struct crtx_event *event) {
	struct crtx_graph *graph;
	
	graph = find_graph_for_event_type(event->type);
	
	if (!graph && !strcmp(event->type, CRTX_EVT_NOTIFICATION)) {
		crtx_init_notification_listeners(&crtx_root->notification_listeners_handle);
		
		graph = find_graph_for_event_type(event->type);
	}
	
	if (!graph) {
		ERROR("did not find graph for event type %s\n", event->type);
		return;
	}
	
	add_event(graph, event);
}

static void new_eventgraph(struct crtx_graph **crtx_graph, char *name, char **event_types) {
	struct crtx_graph *graph;
	int i, ret;
	
	if (crtx_graph && *crtx_graph) {
		ERROR("new_eventgraph: will not overwrite graph ptr\n");
		return;
	}
	
	graph = (struct crtx_graph*) calloc(1, sizeof(struct crtx_graph)); ASSERT(graph);
	
	if (crtx_graph)
		*crtx_graph = graph;
	
	ret = pthread_mutex_init(&graph->mutex, 0); ASSERT(ret >= 0);
	
	ret = pthread_mutex_init(&graph->queue_mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&graph->queue_cond, NULL); ASSERT(ret >= 0);
	
	graph->name = name;
// 	INFO("new eventgraph %s ", graph->name);
	if (event_types) {
		while (event_types[graph->n_types]) {
// 			INFO("%s ", event_types[graph->n_types]);
			graph->n_types++;
		}
		
		graph->types = event_types;
	}
// 	INFO("\n");
	
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
}

void crtx_create_graph(struct crtx_graph **crtx_graph, char *name, char **event_types) {
	new_eventgraph(crtx_graph, name, event_types);
}

void free_eventgraph_intern(struct crtx_graph *egraph, char crtx_shutdown) {
	size_t i;
	struct crtx_task *t, *tnext;
	struct crtx_dll *qe, *qe_next;
	
	DBG("free graph %s t[0]: %s\n", egraph->name, egraph->types?egraph->types[0]:0);
	
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
		free_task(t);
	}
	
	free(egraph);
}

void free_eventgraph(struct crtx_graph *egraph) {
	free_eventgraph_intern(egraph, 0);
}

struct crtx_task *new_task() {
	struct crtx_task *etask = (struct crtx_task*) calloc(1, sizeof(struct crtx_task));
	etask->position = 100;
	
	return etask;
}

void free_task(struct crtx_task *task) {
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

void crtx_init_shutdown() {
	if (crtx_root->shutdown)
		return;
	
	VDBG("init shutdown\n");
	
	crtx_root->shutdown = 1;
	
	if (crtx_root->event_loop.listener) {
		crtx_epoll_stop(crtx_root->event_loop.listener);
	}
	
// 	crtx_finish();
	crtx_threads_stop_all();
	crtx_flush_events();
}

static char handle_shutdown(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_task *t;
	
	// TODO make sure we're only called once
	t = (struct crtx_task *) userdata;
	t->handle = 0;
	
	crtx_init_shutdown();
	
	return 1;
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

void crtx_init() {
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
	
	memset(crtx_root, 0, sizeof(struct crtx_root));
	
	DBG("initialized cortex (PID: %d)\n", getpid());
	
// 	crtx_root->no_threads = 1;
	crtx_root->default_mode = CRTX_PREFER_ELOOP;
	
	INIT_MUTEX(crtx_root->graphs_mutex);
	
	new_eventgraph(&crtx_root->crtx_ctrl_graph, "cortexd.control", crtx_evt_control);
	
	t = new_task();
	t->id = "shutdown";
	t->handle = &handle_shutdown;
	t->userdata = t;
	t->position = 255;
	t->event_type_match = CRTX_EVT_SHUTDOWN;
	add_task(crtx_root->crtx_ctrl_graph, t);
	
	i=0;
	while (static_modules[i].id) {
		DBG("initialize \"%s\"\n", static_modules[i].id);
		
		static_modules[i].init();
		i++;
	}
}

void crtx_finish() {
	unsigned int i;
	
	crtx_init_shutdown();
	
	i=0;
	while (static_modules[i].id) { i++; }
	i--;
	
	if (crtx_root->notification_listeners_handle)
		crtx_finish_notification_listeners(crtx_root->notification_listeners_handle);
	
	// stop threads first
// 	static_modules[0].finish();
	crtx_threads_stop_all();
	// stop controls second
	static_modules[1].finish();
	
	while (i > 1) {
		DBG("finish \"%s\"\n", static_modules[i].id);
		
		static_modules[i].finish();
		i--;
	}
	
	if (crtx_root->event_loop.listener) {
		crtx_free_listener((struct crtx_listener_base *) crtx_root->event_loop.listener);
// 		free(crtx_root->event_loop.listener);
	}
	
	// finish threads module
	static_modules[0].finish();
	
	LOCK(crtx_root->graphs_mutex);
	for (i=0; i < crtx_root->n_graphs; i++) {
		if (crtx_root->graphs[i])
			free_eventgraph_intern(crtx_root->graphs[i], 1);
	}
	free(crtx_root->graphs);
	UNLOCK(crtx_root->graphs_mutex);
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
	if (crtx_root->event_loop.listener)
		crtx_epoll_main(crtx_root->event_loop.listener);
	else
		spawn_thread(0);
}

void crtx_init_notification_listeners(void **data) {
	void *result, *tmp;
	struct crtx_sd_bus_notification_listener *notify_listener;
	struct crtx_readline_listener *rl_listener;
	
	
	*data = calloc(1, 
			sizeof(struct crtx_sd_bus_notification_listener) +
			sizeof(struct crtx_readline_listener));
	tmp = *data;
	
	notify_listener = (struct crtx_sd_bus_notification_listener *) tmp;
	tmp += sizeof(struct crtx_sd_bus_notification_listener);
	
	// create a listener (queue) for notification events
	result = create_listener("sd_bus_notification", notify_listener);
	if (!result) {
		printf("cannot create fanotify listener\n");
	}
	
	rl_listener = (struct crtx_readline_listener *) tmp;
	tmp += sizeof(struct crtx_readline_listener);
	
	// create a listener (queue) for notification events
	result = create_listener("readline", &rl_listener);
	if (!result) {
		printf("cannot create fanotify listener\n");
	}
}

void crtx_finish_notification_listeners(void *data) {
	struct crtx_listener_base *base;
	void *tmp;
	
	tmp = data;
	
	base = (struct crtx_listener_base*) tmp;
	crtx_free_listener(base);
	tmp += sizeof(struct crtx_sd_bus_notification_listener);
	
	base = (struct crtx_listener_base*) tmp;
	crtx_free_listener(base);
	tmp += sizeof(struct crtx_readline_listener);
	
	free(data);
}

struct crtx_event_loop* crtx_get_event_loop() {
	if (!crtx_root->event_loop.listener) {
		struct crtx_listener_base *lbase;
		int ret;
		
// 		crtx_root->event_loop = (struct crtx_event_loop*) calloc(1, sizeof(struct crtx_event_loop));
		
		crtx_root->event_loop.listener = (struct crtx_epoll_listener*) calloc(1, sizeof(struct crtx_epoll_listener));
		
// 		crtx_root->event_loop.listener->no_thread = 1 - crtx_root->event_loop.start_thread;
// 		crtx_root->event_loop.listener->parent.lmode = crtx_root->lmode;
		
		crtx_root->event_loop.add_fd = &crtx_epoll_add_fd;
		crtx_root->event_loop.mod_fd = &crtx_epoll_mod_fd;
		crtx_root->event_loop.del_fd = &crtx_epoll_del_fd;
		
		lbase = create_listener("epoll", crtx_root->event_loop.listener);
		if (!lbase) {
			ERROR("create_listener(epoll) failed\n");
			exit(1);
		}
		
		if (crtx_root->detached_event_loop) {
			ret = crtx_start_listener(lbase);
			if (!ret) {
				ERROR("starting epoll listener failed\n");
				return 0;
			}
		}
	} 
	
	return &crtx_root->event_loop;
}

void crtx_event_set_data(struct crtx_event *event, void *raw_pointer, struct crtx_dict *data_dict, unsigned char n_additional_fields) {
	struct crtx_dict *upgraded_dict;
	struct crtx_dict_item *di;
	
	
	LOCK(event->mutex);
	
	if (raw_pointer || event->data.type == 0) {
		switch (event->data.type) {
			case 0:
			case 'p':
				event->data.key = "raw";
				event->data.type = 'p';
				event->data.size = 0;
				event->data.flags = 0;
				event->data.pointer = raw_pointer;
				break;
			case 'D':
				di = crtx_get_item_by_idx(event->data.dict, 0);
				di->pointer = raw_pointer;
				break;
			default:
				ERROR("invalid event data type %c (%d)\n", event->data.type, event->data.type);
				break;
		}
	}
	
	if (data_dict || n_additional_fields > 0) {
		if (event->data.type == 'p') {
			upgraded_dict = crtx_init_dict(0, 2+n_additional_fields, 0);
			
			di = crtx_get_item_by_idx(upgraded_dict, 0);
			
// 			crtx_dict_copy_item(di, &event->data, 0);
			memcpy(di, &event->data, sizeof(struct crtx_dict_item));
			
			di = crtx_get_item_by_idx(upgraded_dict, 1);
			di->key = "data";
			di->type = 'D';
			di->dict = data_dict;
			
			event->data.key = "data";
			event->data.type = 'D';
			event->data.size = 0;
			event->data.flags = 0;
			event->data.dict = upgraded_dict;
		} else
		if (event->data.type == 'D') {
			if (strcmp(event->data.key, "data"))
				ERROR("invalid event data structure\n");
			
			di = crtx_get_item_by_idx(event->data.dict, 1);
			di->dict = data_dict;
			
			if (event->data.dict->signature_length < 2 + n_additional_fields)
				crtx_resize_dict(event->data.dict, 2 + n_additional_fields);
		}
	}
	
	UNLOCK(event->mutex);
}

char crtx_event_raw2dict(struct crtx_event *event, void *user_data) {
	struct crtx_dict_item *raw2dict;
	crtx_raw_to_dict_t raw2dict_fct;
	
	if (event->data.type != 'D')
		return CRTX_ERROR;
	
	raw2dict = crtx_get_item(event->data.dict, "raw2dict");
	
	if (!raw2dict)
		return CRTX_ERROR;
	
	if (raw2dict->type != 'p')
		return CRTX_ERROR;
	
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
			di = crtx_get_item_by_idx(event->data.dict, 0);
			*raw_pointer = di->pointer;
		} else {
			ERROR("unknown event data type %d\n", event->data.type);
			*raw_pointer = 0;
			return;
		}
	}
	if (dict) {
		if (event->data.type == 'p') {
			int ret;
			
			ret = crtx_event_raw2dict(event, 0);
			if (ret != CRTX_SUCCESS) {
				ERROR("cannot convert raw data\n");
				*dict = 0;
				return;
			}
		} else
		if (event->data.type != 'D') {
			ERROR("unknown event data type %d\n", event->data.type);
			*dict = 0;
			return;
		}
		
		di = crtx_get_item_by_idx(event->data.dict, 1);
		*dict = di->dict;
	}
}

struct crtx_dict_item *crtx_event_get_value_by_key(struct crtx_event *event, char *id, char *key) {
	struct crtx_dict *dict;
	
	crtx_event_get_payload(event, id, 0, &dict);
	
	if (!dict)
		return 0;
	
	return crtx_get_item(dict, key);
}

void *crtx_event_get_ptr(struct crtx_event *event) {
	void *ptr;
	
	crtx_event_get_payload(event, 0, &ptr, 0);
	
	return ptr;
}
