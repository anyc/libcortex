#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <stddef.h>
#include <inttypes.h>

#include "core.h"

// char ** event_types = 0;
// unsigned n_event_types = 0;

struct event_graph **graphs;
unsigned int n_graphs;

struct event_graph *cortexd_graph;
static char * local_event_types[] = { "cortexd/module_initialized", 0 };

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

struct module static_modules[] = {
	{"socket", &socket_init, &socket_finish},
#ifdef STATIC_SD_BUS
	{"sd-bus", &sd_bus_init, &sd_bus_finish},
#endif
#ifdef STATIC_NF_QUEUE
	{"nf-queue", &nf_queue_init, &nf_queue_finish},
#endif
	{"readline", &readline_init, &readline_finish},
	{"fanotify", &crtx_fanotify_init, &crtx_fanotify_finish},
	{"inotify", &crtx_inotify_init, &crtx_inotify_finish},
	{"controls", &controls_init, &controls_finish},
	{0, 0}
};

struct listener_factory listener_factory[] = {
#ifdef STATIC_NF_QUEUE
	{"nf_queue", &new_nf_queue_listener},
#endif
	{"fanotify", &new_fanotify_listener},
	{"inotify", &new_inotify_listener},
	{"socket_server", &new_socket_server_listener},
	{"socket_client", &new_socket_client_listener},
	{"sd_bus_notification", &new_sd_bus_notification_listener},
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

struct listener *create_listener(char *id, void *options) {
	struct listener_factory *l;
	
	l = listener_factory;
	while (l->id) {
		if (!strcmp(l->id, id)) {
			return l->create(options);
		}
		l++;
	}
	
	printf("listener \"%s\" not found\n", id);
	
	return 0;
}

void free_listener(struct listener *listener) {
	if (listener->free) {
		listener->free(listener);
	} else {
		if (listener->graph)
			free_eventgraph(listener->graph);
		
// 		pthread_join(slistener->thread, 0);
		
		free(listener);
	}
}

void traverse_graph_r(struct event_task *ti, struct event *event) {
	void *sessiondata = 0;
	
	printf("execute task %s\n", ti->id);
	
	if (ti->handle)
		ti->handle(event, ti->userdata, &sessiondata);
	
	if (!event->response.raw && ti->next)
		traverse_graph_r(ti->next, event);
	
	if (ti->cleanup)
		ti->cleanup(event, ti->userdata, sessiondata);
}

void crtx_traverse_graph(struct event_graph *graph, struct event *event) {
	traverse_graph_r(graph->tasks, event);
}

void *graph_consumer_main(void *arg) {
	struct thread *self = (struct thread*) arg;
	struct queue_entry *qe;
// 	struct event_task *ti;
	
	while (!self->stop) {
		pthread_mutex_lock(&self->graph->queue_mutex);
		
		while (self->graph->n_queue_entries == 0)
			pthread_cond_wait(&self->graph->queue_cond, &self->graph->queue_mutex);
		
		qe = self->graph->equeue;
		self->graph->equeue = self->graph->equeue->next;
		self->graph->n_queue_entries--;
		
// 		for (ti=self->graph->tasks;ti;ti=ti->next) {
// 			if (ti->handle)
// 				ti->handle(qe->event, ti->userdata);
// 			if (!ti->next)
// 				break;
// 		}
// 		
// 		for (;ti;ti=ti->prev)
// 			if (ti->cleanup)
// 				ti->cleanup(qe->event, ti->userdata);
		
		if (self->graph->tasks)
			traverse_graph_r(self->graph->tasks, qe->event);
		else
			printf("no task in graph %s\n", self->graph->types[0]);
		
// 		pthread_mutex_lock(&qe->event->mutex);
// 		qe->event->refs_before_response--;
// 		
// 		if (qe->event->refs_before_response == 0)
// 			pthread_cond_broadcast(&qe->event->response_cond);
// 		
// 		pthread_mutex_unlock(&qe->event->mutex);
		dereference_event_response(qe->event);
		
		dereference_event_release(qe->event);
		
		free(qe);
		
		pthread_mutex_unlock(&self->graph->queue_mutex);
	}
	
	return 0;
}

void spawn_thread(struct event_graph *graph) {
	graph->n_consumers++;
	graph->consumers = (struct thread*) realloc(graph->consumers, sizeof(struct thread)*graph->n_consumers); ASSERT(graph->consumers);
	
	graph->consumers[graph->n_consumers-1].stop = 0;
	graph->consumers[graph->n_consumers-1].graph = graph;
	pthread_create(&graph->consumers[graph->n_consumers-1].handle, NULL, &graph_consumer_main, &graph->consumers[graph->n_consumers-1]);
}

void add_task(struct event_graph *graph, struct event_task *task) {
	struct event_task *ti, *last;
	
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

void event_ll_add(struct queue_entry **list, struct event *event) {
	struct queue_entry *eit;
	
	for (eit=*list; eit && eit->next; eit = eit->next) {}
	
	if (eit) {
		eit->next = (struct queue_entry*) malloc(sizeof(struct queue_entry));
		eit = eit->next;
	} else {
		*list = (struct queue_entry*) malloc(sizeof(struct queue_entry));
		eit = *list;
	}
	eit->event = event;
	eit->next = 0;
}

void add_event(struct event_graph *graph, struct event *event) {
	
	reference_event_release(event);
	reference_event_response(event);
	
	pthread_mutex_lock(&graph->mutex);
	
	printf("new event %s\n", event->type);
	
	event_ll_add(&graph->equeue, event);
	
// 	pthread_mutex_lock(&event->mutex);
// 	event->refs_before_response++;
// 	pthread_mutex_unlock(&event->mutex);
	
	graph->n_queue_entries++;
	
	if (graph->n_consumers == 0)
		spawn_thread(graph);
	
	pthread_cond_signal(&graph->queue_cond);
	
	pthread_mutex_unlock(&graph->mutex);
}

// void add_event_sync(struct event_graph *graph, struct event *event) {
// 	struct event *eit;
// 	
// 	pthread_mutex_lock(&graph->mutex);
// 	
// 	printf("new event %s\n", event->type);
// 	
// 	for (eit=graph->equeue; eit && eit->next; eit = eit->next) {}
// 	
// 	if (eit) {
// 		eit->next = event;
// 	} else {
// 		graph->equeue = event;
// 	}
// 	event->next = 0;
// 	graph->n_queue_entries++;
// 	
// 	if (graph->n_consumers == 0)
// 		spawn_thread(graph);
// 	
// 	pthread_cond_signal(&graph->queue_cond);
// 	
// 	pthread_mutex_unlock(&graph->mutex);
// 	
// 	
// 	pthread_mutex_lock(&event->mutex);
// 	
// 	while (event->response == 0)
// 		pthread_cond_wait(&event->cond, &event->mutex);
// 	
// 	pthread_mutex_unlock(&event->mutex);
// }

struct event *create_event(char *type, void *data, size_t data_size) {
	struct event *event;
	
	event = new_event();
	event->type = type;
	event->data.raw = data;
	event->data.raw_size = data_size;
	
	return event;
}

// struct event *create_response(struct event *event, void *data, size_t data_size) {
// 	struct event *response;
// 	
// 	response = create_event(event->type, data, data_size);
// 	response->original_event = event;
// 	
// 	return response;
// }

void init_signal(struct signal *signal) {
	int ret;
	
	ret = pthread_mutex_init(&signal->mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&signal->cond, NULL); ASSERT(ret >= 0);
	
	signal->local_condition = 0;
	signal->condition = &signal->local_condition;
}

struct signal *new_signal() {
	struct signal *signal;
	
	signal = (struct signal*) calloc(1, sizeof(struct signal));
	
	init_signal(signal);
	
	return signal;
}

void wait_on_signal(struct signal *signal) {
	pthread_mutex_lock(&signal->mutex);
	
	while (!*signal->condition)
		pthread_cond_wait(&signal->cond, &signal->mutex);
	
	pthread_mutex_unlock(&signal->mutex);
}

void send_signal(struct signal *s, char brdcst) {
	pthread_mutex_lock(&s->mutex);
	
	if (!*s->condition)
		*s->condition = 1;
	
	if (brdcst)
		pthread_cond_broadcast(&s->cond);
	else
		pthread_cond_signal(&s->cond);
	
	pthread_mutex_unlock(&s->mutex);
}

void free_signal(struct signal *s) {
	free(s);
}

void reference_event_release(struct event *event) {
	pthread_mutex_lock(&event->mutex);
	event->refs_before_release++;
	pthread_mutex_unlock(&event->mutex);
}

void dereference_event_release(struct event *event) {
	pthread_mutex_lock(&event->mutex);
	printf("deref release %d %s\n", event->refs_before_release, event->type);
	if (event->refs_before_release > 0)
		event->refs_before_release--;
	
	if (event->refs_before_release == 0) {
// 		pthread_cond_broadcast(&event->release_cond);
		free_event(event);
		return;
	}
	
	pthread_mutex_unlock(&event->mutex);
}

void reference_event_response(struct event *event) {
	pthread_mutex_lock(&event->mutex);
	event->refs_before_response++;
	pthread_mutex_unlock(&event->mutex);
}

void dereference_event_response(struct event *event) {
	pthread_mutex_lock(&event->mutex);
	
	if (event->refs_before_response > 0)
		event->refs_before_response--;
	
	if (event->refs_before_response == 0)
		pthread_cond_broadcast(&event->response_cond);
	
	pthread_mutex_unlock(&event->mutex);
}

void wait_on_event(struct event *event) {
	pthread_mutex_lock(&event->mutex);
	
	while (event->refs_before_response > 0)
		pthread_cond_wait(&event->response_cond, &event->mutex);
	
	pthread_mutex_unlock(&event->mutex);
}

struct event *new_event() {
	struct event *event;
	int ret;
	
	event = (struct event*) calloc(1, sizeof(struct event));
// 	event->id = (uint64_t) (uintptr_t) event;
	
	ret = pthread_mutex_init(&event->mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&event->response_cond, NULL); ASSERT(ret >= 0);
// 	ret = pthread_cond_init(&event->release_cond, NULL); ASSERT(ret >= 0);
	
	return event;
}

void free_event_data(struct event_data *ed) {
	if (ed->raw)
		free(ed->raw);
	if (ed->dict)
		free_dict(ed->dict);
}

void free_event(struct event *event) {
	printf("free %s\n", event->type);
	
	if (event->cb_before_release)
		event->cb_before_release(event);
	
	free_event_data(&event->data);
	free_event_data(&event->response);
	
	free(event);
}

struct event_graph *find_graph_for_event_type(char *event_type) {
	struct event_graph *graph;
	unsigned int i, j;
	
	graph = 0;
	for (i=0; i < n_graphs && !graph; i++) {
		for (j=0; j < graphs[i]->n_types; j++) {
			if (!strcmp(graphs[i]->types[j], event_type)) {
				graph = graphs[i];
				break;
			}
		}
	}
	
	return graph;
}

struct event_graph *get_graph_for_event_type(char *event_type, char **event_types) {
	struct event_graph *graph;
	
	graph = find_graph_for_event_type(event_type);
	if (!graph) {
		new_eventgraph(&graph, event_types);
	}
	
	return graph;
}

void add_raw_event(struct event *event) {
	struct event_graph *graph;
	
	graph = find_graph_for_event_type(event->type);
	
	if (!graph) {
		printf("did not find graph for event type %s\n", event->type);
		return;
	}
	
	add_event(graph, event);
}

void new_eventgraph(struct event_graph **event_graph, char **event_types) {
	struct event_graph *graph;
	int ret;
	
	graph = (struct event_graph*) calloc(1, sizeof(struct event_graph)); ASSERT(graph);
	
	*event_graph = graph;
	
	ret = pthread_mutex_init(&graph->mutex, 0); ASSERT(ret >= 0);
	
	ret = pthread_mutex_init(&graph->queue_mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&graph->queue_cond, NULL); ASSERT(ret >= 0);
	
	printf("new eventgraph ");
	if (event_types) {
		while (event_types[graph->n_types]) {
			printf("%s ", event_types[graph->n_types]);
			graph->n_types++;
		}
		
		graph->types = event_types;
	}
	printf("\n");
	
	n_graphs++;
	graphs = (struct event_graph**) realloc(graphs, sizeof(struct event_graph*)*n_graphs);
	graphs[n_graphs-1] = graph;
}

void free_eventgraph(struct event_graph *egraph) {
	size_t i;
	struct event_task *t, *tnext;
	struct queue_entry *qe, *qe_next;
	
	for (qe = egraph->equeue; qe; qe=qe_next) {
		free_event(qe->event);
		qe_next = qe->next;
		free(qe);
	}
	
// 	for (i=0; i<egraph->n_consumers; i++) {
// 		pthread_join(egraph->consumers[i].handle, 0);
// 	}
	free(egraph->consumers);
	
	for (t = egraph->tasks; t; t=tnext) {
		tnext = t->next;
		free_task(t);
	}
	
	for (i=0; i < n_graphs; i++) {
		if (graphs[i] == egraph) {
			graphs[i] = 0;
			break;
		}
	}
	
	free(egraph);
}

// void add_event_type(char *event_type) {
// 	n_event_types++;
// 	event_types = (char**) realloc(event_types, sizeof(char*)*n_event_types);
// 	event_types[n_event_types-1] = event_type;
// }

struct event_task *new_task() {
	struct event_task *etask = (struct event_task*) calloc(1, sizeof(struct event_task));
	etask->position = 100;
	
	return etask;
}

void free_task(struct event_task *task) {
	struct event_task *t, *prev;
	
	prev=0;
// 	if (task->graph) {
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
// 	}
	
	free(task);
}

void cortex_init() {
	unsigned int i;
	
// 	i=0;
// 	while (local_event_types[i]) {
// 		add_event_type(local_event_types[i]);
// 		i++;
// 	}
	
	new_eventgraph(&cortexd_graph, local_event_types);
	
	i=0;
	while (static_modules[i].id) {
		printf("initialize \"%s\"\n", static_modules[i].id);
		
		static_modules[i].init();
		i++;
	}
	
// 	cortexd_graph->tasks = etask;
}

void cortex_finish() {
	unsigned int i;
	
	i=0;
	while (static_modules[i].id) {
		printf("finish \"%s\"\n", static_modules[i].id);
		
		static_modules[i].finish();
		i++;
	}
}

void print_tasks(struct event_graph *graph) {
	struct event_task *e;
	
	for (e=graph->tasks; e; e=e->next) {
		printf("%u %s\n", e->position, e->id);
	}
}

void hexdump(unsigned char *buffer, size_t index) {
	size_t i;
	
	for (i=0;i<index;i++) {
		printf("%02x ", buffer[i]);
	}
	printf("\n");
}

typedef char (*event_notifier_filter)(struct event *event);
typedef void (*event_notifier_cb)(struct event *event);
struct event_notifier {
	event_notifier_filter filter;
	event_notifier_cb callback;
	
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

void event_notifier_task(struct event *event, void *userdata, void **sessiondata) {
	struct event_notifier *en = (struct event_notifier*) userdata;
	
	if (en->filter(event)) {
		en->callback(event);
		
		pthread_mutex_lock(&en->mutex);
		pthread_cond_broadcast(&en->cond);
		pthread_mutex_unlock(&en->mutex);
	}
}

void wait_on_notifier(struct event_notifier *en) {
	pthread_mutex_lock(&en->mutex);
	
	pthread_cond_wait(&en->cond, &en->mutex);
	
	pthread_mutex_unlock(&en->mutex);
}


struct event_task *new_event_notifier(struct event_graph *graph, event_notifier_filter filter, event_notifier_cb callback) {
	struct event_task *task;
	struct event_notifier *en;
	int ret;
	
	en = (struct event_notifier*) calloc(1, sizeof(struct event_notifier));
	en->filter = filter;
	en->callback = callback;
	
	ret = pthread_mutex_init(&en->mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&en->cond, NULL); ASSERT(ret >= 0);
	
	task = new_task();
	task->id = "event_notifier";
	task->position = 110;
	task->userdata = en;
	task->handle = &event_notifier_task;
	
	add_task(graph, task);
	
	return task;
}

struct data_struct * crtx_create_dict(char *signature, ...) {
	struct data_struct *ds;
	struct data_item *di;
	char *s;
	size_t size;
	va_list va;
	
	size = strlen(signature) * sizeof(struct data_item) + sizeof(struct data_struct);
	ds = (struct data_struct*) malloc(size);
	ds->signature = signature;
	ds->size = size;
	ds->signature_length = 0;
	
	va_start(va, signature);
	
	s = signature;
	di = ds->items;
	while (*s) {
		di->type = *s;
		di->key = va_arg(va, char*);
		
		switch (*s) {
			case 'u':
				di->uint32 = va_arg(va, uint32_t);
				break;
			case 's':
				di->string = va_arg(va, char*);
				break;
			case 'D':
				di->ds = va_arg(va, struct data_struct*);
				break;
			default:
				printf("unknown literal '%c' in signature \"%s\"\n", *s, signature);
				return 0;
		}
		
		di->size = va_arg(va, size_t);
		di->flags = va_arg(va, int);
		
		di++;
		s++;
		ds->signature_length++;
	}
	di--;
	di->flags |= DIF_LAST;
	
	va_end(va);
	
	return ds;
}


void free_dict(struct data_struct *ds) {
	struct data_item *di;
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
	
// 	free(ds->signature);
	free(ds);
}

void crtx_print_dict(struct data_struct *ds) {
	char *s;
	struct data_item *di;
	uint8_t i;
	
	printf("sign: %s (%zu==%u)\n", ds->signature, strlen(ds->signature), ds->signature_length);
	
	i=0;
	s = ds->signature;
	di = ds->items;
	while (*s) {
		printf("%u: %s = ", i, di->key);
		
		if (*s != di->type)
			printf("error type %c != %c\n", *s, di->type);
		
		switch (di->type) {
			case 'u': printf("(uint32_t) %d\n", di->uint32); break;
			case 'i': printf("(int32_t) %d\n", di->int32); break;
			case 's': printf("(char*) %s\n", di->string); break;
			case 'D': printf("(dict) TODO\n");
		}
		
		s++;
		di++;
		i++;
	}
}

char crtx_get_value(struct data_struct *ds, char *key, void *buffer, size_t buffer_size) {
	char *s;
	struct data_item *di;
	
	s = ds->signature;
	di = ds->items;
	while (*s) {printf("%s %s %c %c\n", di->key, key, *s, di->type);
		if (!strcmp(di->key, key)) {
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
						printf("error, size does not match type: %zu != %zu\n", buffer_size, sizeof(void*));
					break;
				default:
					printf("error, unimplemented value type %c\n", di->type);
			}
		}
		
		s++;
		di++;
	}
	
	return 0;
}