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

#include "socket.h"
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


struct crtx_graph **graphs;
unsigned int n_graphs;
MUTEX_TYPE graphs_mutex;

static char * crtx_evt_control[] = { CRTX_EVT_MOD_INIT, CRTX_EVT_SHUTDOWN, 0 };
struct crtx_graph *crtx_ctrl_graph;


struct crtx_module static_modules[] = {
	{"threads", &crtx_threads_init, &crtx_threads_finish},
	{"signals", &crtx_signals_init, &crtx_signals_finish},
	{"socket", &crtx_socket_init, &crtx_socket_finish},
#ifdef STATIC_SD_BUS
	{"sd-bus", &crtx_sd_bus_init, &crtx_sd_bus_finish},
#endif
#ifdef STATIC_NF_QUEUE
	{"nf-queue", &crtx_nf_queue_init, &crtx_nf_queue_finish},
#endif
	{"readline", &crtx_readline_init, &crtx_readline_finish},
	{"fanotify", &crtx_fanotify_init, &crtx_fanotify_finish},
	{"inotify", &crtx_inotify_init, &crtx_inotify_finish},
	{"sd_bus_notification", &crtx_sdbus_notification_init, &crtx_sdbus_notification_finish},
	{"controls", &crtx_controls_init, &crtx_controls_finish},
	{0, 0}
};

struct crtx_listener_repository listener_factory[] = {
#ifdef STATIC_NF_QUEUE
	{"nf_queue", &crtx_new_nf_queue_listener},
#endif
	{"fanotify", &crtx_new_fanotify_listener},
	{"inotify", &crtx_new_inotify_listener},
	{"socket_server", &crtx_new_socket_server_listener},
	{"socket_client", &crtx_new_socket_client_listener},
	{"sd_bus_notification", &crtx_new_sd_bus_notification_listener},
	{"signals", &crtx_new_signals_listener},
	{"readline", &crtx_new_readline_listener},
	{0, 0}
};

char *crtx_evt_notification[] = { CRTX_EVT_NOTIFICATION, 0 };
char *crtx_evt_inbox[] = { CRTX_EVT_INBOX, 0 };
char *crtx_evt_outbox[] = { CRTX_EVT_OUTBOX, 0 };


char *stracpy(char *str, size_t *str_length) {
	char *r;
	size_t length;
	
	if (!str_length || *str_length == 0) {
		length = strlen(str);
		if (str_length && *str_length == 0)
			*str_length = length;
	} else {
		length = *str_length;
	}
	
	r = (char*) malloc(length+1);
	snprintf(r, length+1, "%s", str);
	
	return r;
}

struct crtx_listener_base *create_listener(char *id, void *options) {
	struct crtx_listener_repository *l;
	
	l = listener_factory;
	while (l->id) {
		if (!strcmp(l->id, id)) {
			return l->create(options);
		}
		l++;
	}
	
	ERROR("listener \"%s\" not found\n", id);
	
	return 0;
}

void free_listener(struct crtx_listener_base *listener) {
	if (listener->free)
		listener->free(listener);
	
	if (listener->graph)
		free_eventgraph(listener->graph);
		
// 		pthread_join(slistener->thread, 0);
		
// 		free(listener);
// 	}
}

void traverse_graph_r(struct crtx_task *ti, struct crtx_event *event) {
	void *sessiondata = 0;
	
	if (ti->handle && (!ti->event_type_match || !strcmp(ti->event_type_match, event->type))) {
		INFO("execute task %s with event %s (%p)\n", ti->id, event->type, event);
		
		ti->handle(event, ti->userdata, &sessiondata);
	}
	
	if (ti->next)
		traverse_graph_r(ti->next, event);
	
	if (ti->cleanup)
		ti->cleanup(event, ti->userdata, sessiondata);
}

void crtx_traverse_graph(struct crtx_graph *graph, struct crtx_event *event) {
	traverse_graph_r(graph->tasks, event);
}

void *graph_consumer_main(void *arg) {
	struct crtx_event_ll *qe;
	struct crtx_graph *graph = (struct crtx_graph*) arg;
	
	LOCK(graph->queue_mutex);
	
// 	while (graph->n_queue_entries == 0)
// 		pthread_cond_wait(&graph->queue_cond, &graph->queue_mutex);
	
	while (!graph->stop) {
		for (qe = graph->equeue; qe && qe->in_process && qe->event->error; qe = qe->next) {}
		if (qe && !qe->in_process && !qe->event->error) {
			qe->in_process = 1;
			break;
		}
		pthread_cond_wait(&graph->queue_cond, &graph->queue_mutex);
	}
// 	qe = graph->equeue;
// 	graph->equeue = graph->equeue->next;
// 	graph->n_queue_entries--;
	
	UNLOCK(graph->queue_mutex);
	
	if (graph->tasks)
		traverse_graph_r(graph->tasks, qe->event);
	else
		INFO("no task in graph %s\n", graph->types[0]);
	
	dereference_event_response(qe->event);
	
	dereference_event_release(qe->event);
	
	LOCK(graph->queue_mutex);
	if (qe->prev)
		qe->prev->next = qe->next;
	else
		graph->equeue = qe->next;
	if (qe->next)
		qe->next->prev = qe->prev;
	UNLOCK(graph->queue_mutex);
	
	free(qe);
	
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

void event_ll_add(struct crtx_event_ll **list, struct crtx_event *event) {
	struct crtx_event_ll *eit;
	
	for (eit=*list; eit && eit->next; eit = eit->next) {}
	
	if (eit) {
		eit->next = (struct crtx_event_ll*) malloc(sizeof(struct crtx_event_ll));
		eit->next->prev = eit;
		eit = eit->next;
	} else {
		*list = (struct crtx_event_ll*) malloc(sizeof(struct crtx_event_ll));
		eit = *list;
		eit->prev = 0;
	}
	eit->event = event;
	eit->in_process = 0;
	eit->next = 0;
}

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
	unsigned int new_n_consumers;
	
	reference_event_release(event);
	reference_event_response(event);
	
	pthread_mutex_lock(&graph->mutex);
	
	DBG("add event %s (%p) to graph\n", event->type, event);
	
	if (!graph->tasks) {
		INFO("dropping event as graph is empty\n");
		
		pthread_mutex_unlock(&graph->mutex);
		return;
	}
	
	LOCK(graph->queue_mutex);
	
	event_ll_add(&graph->equeue, event);
	
	graph->n_queue_entries++;
	
	pthread_cond_signal(&graph->queue_cond);
	
	UNLOCK(graph->queue_mutex);
	
	new_n_consumers = ATOMIC_FETCH_ADD(graph->n_consumers, 1);
	if (!graph->n_max_consumers || new_n_consumers < graph->n_max_consumers) {
		struct crtx_thread *t;
		
		t = get_thread(&graph_consumer_main, graph, 0);
		if (t) {
			t->on_finish = &graph_on_thread_finish;
			t->on_finish_data = graph;
			t->do_stop = &graph_consumer_stop;
			
			start_thread(t);
		} else {
			ATOMIC_FETCH_SUB(graph->n_consumers, 1);
		}
	} else {
		ATOMIC_FETCH_SUB(graph->n_consumers, 1);
	}
	
	pthread_mutex_unlock(&graph->mutex);
}

struct crtx_event *create_event(char *type, void *data, size_t data_size) {
	struct crtx_event *event;
	
	event = new_event();
	event->type = type;
	event->data.raw = data;
	event->data.raw_size = data_size;
	
	return event;
}

void reference_event_release(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	event->refs_before_release++;
	DBG("ref release of event %s (%p) (now %d)\n", event->type, event, event->refs_before_release);
	
	pthread_mutex_unlock(&event->mutex);
}

void dereference_event_release(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	DBG("deref release of event %s (%p) (remaining %d)\n", event->type, event, event->refs_before_release);
	
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
		DBG("ref response of event %s (%p) (now %d)\n", event->type, event, event->refs_before_response);
	}
	
	pthread_mutex_unlock(&event->mutex);
}

void dereference_event_response(struct crtx_event *event) {
	pthread_mutex_lock(&event->mutex);
	
	DBG("ref response of event %s (%p) (remaining %d)\n", event->type, event, event->refs_before_response);
	
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

struct crtx_event *new_event() {
	struct crtx_event *event;
	int ret;
	
	event = (struct crtx_event*) calloc(1, sizeof(struct crtx_event));
	
	ret = pthread_mutex_init(&event->mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&event->response_cond, NULL); ASSERT(ret >= 0);
	ret = pthread_cond_init(&event->release_cond, NULL); ASSERT(ret >= 0);
	
	return event;
}

void free_event_data(struct crtx_event_data *ed) {
	if (ed->raw)
		free(ed->raw);
	if (ed->dict)
		free_dict(ed->dict);
}

void crtx_invalidate_event(struct crtx_event *event) {
	printf("invalidate %p\n", event);
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
	
	free_event_data(&event->data);
	free_event_data(&event->response);
	
	free(event);
}

struct crtx_graph *find_graph_for_event_type(char *event_type) {
	unsigned int i, j;
	
	for (i=0; i < n_graphs; i++) {
		for (j=0; j < graphs[i]->n_types; j++) {
			if (!strcmp(graphs[i]->types[j], event_type)) {
				return graphs[i];
			}
		}
	}
	
	return 0;
}

struct crtx_graph *find_graph_by_name(char *name) {
	unsigned int i;
	
	for (i=0; i < n_graphs; i++) {
		if (!strcmp(graphs[i]->name, name)) {
			return graphs[i];
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
	
	if (!graph) {
		ERROR("did not find graph for event type %s\n", event->type);
		return;
	}
	
	add_event(graph, event);
}

void new_eventgraph(struct crtx_graph **crtx_graph, char *name, char **event_types) {
	struct crtx_graph *graph;
	int i, ret;
	
	graph = (struct crtx_graph*) calloc(1, sizeof(struct crtx_graph)); ASSERT(graph);
	
	if (crtx_graph)
		*crtx_graph = graph;
	
	ret = pthread_mutex_init(&graph->mutex, 0); ASSERT(ret >= 0);
	
	ret = pthread_mutex_init(&graph->queue_mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&graph->queue_cond, NULL); ASSERT(ret >= 0);
	
	INFO("new eventgraph ");
	if (event_types) {
		while (event_types[graph->n_types]) {
			INFO("%s ", event_types[graph->n_types]);
			graph->n_types++;
		}
		
		graph->types = event_types;
	}
	INFO("\n");
	
	LOCK(graphs_mutex);
	for (i=0; i < n_graphs; i++) {
		if (graphs[i] == 0) {
			graphs[i] = graph;
			break;
		}
	}
	if (i == n_graphs) {
		n_graphs++;
		graphs = (struct crtx_graph**) realloc(graphs, sizeof(struct crtx_graph*)*n_graphs);
		graphs[n_graphs-1] = graph;
	}
	UNLOCK(graphs_mutex);
}

void free_eventgraph_intern(struct crtx_graph *egraph, char crtx_shutdown) {
	size_t i;
	struct crtx_task *t, *tnext;
	struct crtx_event_ll *qe, *qe_next;
	
	DBG("free graph %s t[0]: %s\n", egraph->name, egraph->types?egraph->types[0]:0);
	
	if (!crtx_shutdown) {
		LOCK(graphs_mutex);
		
		for (i=0; i < n_graphs; i++) {
			if (graphs[i] == egraph) {
				graphs[i] = 0;
				break;
			}
		}
		
		UNLOCK(graphs_mutex);
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
	
	for (i=0; i<n_graphs; i++) {
		struct crtx_event_ll *qe, *qe_next;
		
		if (!graphs[i])
			continue;
		
		LOCK(graphs[i]->queue_mutex);
		printf("flush %s\n", graphs[i]->types? graphs[i]->types[0]: 0);
		for (qe = graphs[i]->equeue; qe ; qe = qe_next) {
			qe_next = qe->next;
// 			graphs[i]->n_queue_entries--;
			
			crtx_invalidate_event(qe->event);
			
// 			free(qe);
		}
// 		graphs[i]->equeue = 0;
		
		UNLOCK(graphs[i]->queue_mutex);
	}
}

static void handle_shutdown(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_task *t;
	
	INFO("shutdown crtx\n");
	
	// TODO make sure we're only called once
	t = (struct crtx_task *) userdata;
	t->handle = 0;
	
// 	crtx_finish();
	crtx_threads_stop();
	crtx_flush_events();
}

void crtx_init() {
	struct crtx_task *t;
	unsigned int i;
	
	DBG("initialized cortex (PID: %d)\n", getpid());
	
	INIT_MUTEX(graphs_mutex);
	
	new_eventgraph(&crtx_ctrl_graph, "cortexd.control", crtx_evt_control);
	
	t = new_task();
	t->id = "shutdown";
	t->handle = &handle_shutdown;
	t->userdata = t;
	t->position = 255;
	t->event_type_match = CRTX_EVT_SHUTDOWN;
	add_task(crtx_ctrl_graph, t);
	
	i=0;
	while (static_modules[i].id) {
		DBG("initialize \"%s\"\n", static_modules[i].id);
		
		static_modules[i].init();
		i++;
	}
}

void crtx_finish() {
	unsigned int i;
	
	i=0;
	while (static_modules[i].id) { i++; }
	i--;
	
	// stop threads first
	static_modules[0].finish();
	
	while (i > 0) {
		DBG("finish \"%s\"\n", static_modules[i].id);
		
		static_modules[i].finish();
		i--;
	}
	
	LOCK(graphs_mutex);
	for (i=0; i < n_graphs; i++) {
		if (graphs[i])
			free_eventgraph_intern(graphs[i], 1);
	}
	free(graphs);
	UNLOCK(graphs_mutex);
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

char crtx_fill_data_item_va(struct crtx_dict_item *di, char type, va_list va) {
// 	if (ds) {
// 		size_t idx;
// 		
// 		idx = di - ds->items;
// 		printf("idx %zu\n", idx);
// 		if (ds->signature[idx] != type) {
// 			printf("type mismatch %c %c\n", ds->signature[idx], type);
// 			return -1;
// 		}
// 	}
	
	di->type = type;
	di->key = va_arg(va, char*);
	
	switch (di->type) {
		case 'u':
			di->uint32 = va_arg(va, uint32_t);
			
			di->size = va_arg(va, size_t);
			if (di->size > 0 && di->size != sizeof(uint32_t)) {
				ERROR("size mismatch %zu %zu\n", di->size, sizeof(uint32_t));
				return 0;
			}
			break;
		case 's':
			di->string = va_arg(va, char*);
			
			di->size = va_arg(va, size_t) + 1; // add space for \0 delimiter
			break;
		case 'D':
			di->ds = va_arg(va, struct crtx_dict*);
			di->size = va_arg(va, size_t);
			break;
		default:
			ERROR("unknown literal '%c'\n", di->type);
			return -1;
	}
	
	di->flags = va_arg(va, int);
	
	return 0;
}

char crtx_fill_data_item(struct crtx_dict_item *di, char type, ...) {
	va_list va;
	char ret;
	
	va_start(va, type);
	
	ret = crtx_fill_data_item_va(di, type, va);
	
	va_end(va);
	
	return ret;
}

struct crtx_dict * crtx_init_dict(char *signature) {
	struct crtx_dict *ds;
	size_t size;
	
	size = strlen(signature) * sizeof(struct crtx_dict_item) + sizeof(struct crtx_dict);
	ds = (struct crtx_dict*) malloc(size);
	
	ds->signature = signature;
	ds->size = size;
	ds->signature_length = 0;
	
	return ds;
}

struct crtx_dict * crtx_create_dict(char *signature, ...) {
	struct crtx_dict *ds;
	struct crtx_dict_item *di;
	char *s, ret;
	va_list va;
	
	
	ds = crtx_init_dict(signature);
	
	va_start(va, signature);
	
	s = signature;
	di = ds->items;
	while (*s) {
		ret = crtx_fill_data_item_va(di, *s, va);
		if (ret < 0)
			return 0;
		
		di++;
		s++;
		ds->signature_length++;
	}
	di--;
	di->flags |= DIF_LAST;
	
	va_end(va);
	
	return ds;
}


void free_dict(struct crtx_dict *ds) {
	struct crtx_dict_item *di;
	char *s;
	
	if (!ds)
		return;
	
	s = ds->signature;
	di = ds->items;
	while (*s) {
		if (di->flags & DIF_KEY_ALLOCATED)
			free(di->key);
		
		switch (di->type) {
			case 's':
				if (!(di->flags & DIF_DATA_UNALLOCATED))
					free(di->string);
				break;
			case 'D':
				free_dict(di->ds);
				break;
		}
		
		if (DIF_IS_LAST(di))
			break;
		
		di++;
		s++;
	}
	
	free(ds);
}

void crtx_print_dict_rec(struct crtx_dict *ds, unsigned char level) {
	char *s;
	struct crtx_dict_item *di;
	uint8_t i,j;
	
	if (!ds) {
		ERROR("error, trying to print non-existing dict\n");
		return;
	}
	
	for (j=0;j<level;j++) printf("  ");
	INFO("sign: %s (%zu==%u)\n", ds->signature, strlen(ds->signature), ds->signature_length);
	
	i=0;
	s = ds->signature;
	di = ds->items;
	while (*s) {
		for (j=0;j<level;j++) INFO("  "); // indent
		INFO("%u: %s = ", i, di->key);
		
		if (*s != di->type)
			INFO("error type %c != %c\n", *s, di->type);
		
		switch (di->type) {
			case 'u': INFO("(uint32_t) %d\n", di->uint32); break;
			case 'i': INFO("(int32_t) %d\n", di->int32); break;
			case 's': INFO("(char*) %s\n", di->string); break;
			case 'D':
				INFO("(dict) \n");
				if (di->ds)
					crtx_print_dict_rec(di->ds, level+1);
				break;
		}
		
		s++;
		di++;
		i++;
	}
}

void crtx_print_dict(struct crtx_dict *ds) {
	crtx_print_dict_rec(ds, 0);
}

char crtx_copy_value(struct crtx_dict_item *di, void *buffer, size_t buffer_size) {
	switch (di->type) {
		case 'u':
		case 'i':
			if (buffer_size == sizeof(uint32_t)) {
				memcpy(buffer, &di->uint32, buffer_size);
				return 1;
			}
		case 's':
		case 'D':
			if (buffer_size == sizeof(void*)) {
				memcpy(buffer, &di->pointer, buffer_size);
				return 1;
			} else
				ERROR("error, size does not match type: %zu != %zu\n", buffer_size, sizeof(void*));
			break;
		default:
			ERROR("error, unimplemented value type %c\n", di->type);
			return 0;
	}
	
	return 0;
}

struct crtx_dict_item *crtx_get_first_item(struct crtx_dict *ds) {
	return ds->items;
}

struct crtx_dict_item *crtx_get_next_item(struct crtx_dict_item *di) {
	return di+1;
}

char crtx_get_value(struct crtx_dict *ds, char *key, void *buffer, size_t buffer_size) {
	char *s;
	struct crtx_dict_item *di;
	
	if (!ds)
		return 0;
	
	s = ds->signature;
	di = ds->items;
	while (*s) {
		if (!strcmp(di->key, key)) {
			return crtx_copy_value(di, buffer, buffer_size);
		}
		
		s++;
		di++;
	}
	
	return 0;
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

void *crtx_copy_raw_data(struct crtx_event_data *data) {
	void *new_data;
	
	new_data = malloc(data->raw_size);
	memcpy(new_data, data->raw, data->raw_size);
	
	return new_data;
}

// void crtx_fprintf(int fd, char *fmt, ...) {
	// check if last char is \n
// }

void crtx_loop() {
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
		return;
	}
	
	rl_listener = (struct crtx_readline_listener *) tmp;
	tmp += sizeof(struct crtx_readline_listener);
	
	// create a listener (queue) for notification events
	result = create_listener("readline", &rl_listener);
	if (!result) {
		printf("cannot create fanotify listener\n");
		return;
	}
}

void crtx_finish_notification_listeners(void *data) {
	struct crtx_listener_base *base;
	void *tmp;
	
	tmp = data;
	
	base = (struct crtx_listener_base*) tmp;
	free_listener(base);
	tmp += sizeof(struct crtx_sd_bus_notification_listener);
	
	base = (struct crtx_listener_base*) tmp;
	free_listener(base);
	tmp += sizeof(struct crtx_readline_listener);
	
	free(data);
}