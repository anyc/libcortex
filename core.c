#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

char ** event_types = 0;
unsigned n_event_types = 0;

struct event_graph **graphs;
unsigned int n_graphs;

struct event_graph *cortexd_graph;
static char * local_event_types[] = { "cortexd_enable", 0 };

#include "socket.h"
#ifdef STATIC_SD_BUS
#include "sd_bus.h"
#endif
#include "nf_queue.h"
#include "readline.h"
#include "plugins.h"

struct module static_modules[] = {
	{"socket", &socket_init, &socket_finish},
#ifdef STATIC_SD_BUS
	{"sd-bus", &sd_bus_init, &sd_bus_finish},
#endif
	{"nf-queue", &nf_queue_init, &nf_queue_finish},
	{"readline", &readline_init, &readline_finish},
	{"plugins", &plugins_init, &plugins_finish},
	{0, 0}
};

struct listener static_listeners[] = {
	{"nf_queue", &new_nf_queue_listener},
	{0, 0}
};

void *create_listener(char *id, void *options) {
	struct listener *l;
	
	l = static_listeners;
	while (l->id) {
		if (!strcmp(l->id, id)) {
			return l->create(options);
		}
		l++;
	}
	
	return 0;
}

void *graph_consumer_main(void *arg) {
	struct thread *self = (struct thread*) arg;
	struct queue_entry *qe;
	struct event_task *ti;
	
	while (!self->stop) {
		pthread_mutex_lock(&self->graph->queue_mutex);
		
		while (self->graph->n_queue_entries == 0)
			pthread_cond_wait(&self->graph->queue_cond, &self->graph->queue_mutex);
		
		qe = self->graph->equeue;
		self->graph->equeue = self->graph->equeue->next;
		self->graph->n_queue_entries--;
		
		for (ti=self->graph->tasks;ti;ti=ti->next) {
			if (ti->handle)
				ti->handle(qe->event, ti->userdata);
			if (!ti->next)
				break;
		}
		
		for (;ti;ti=ti->prev)
			if (ti->cleanup)
				ti->cleanup(qe->event, ti->userdata);
		
		pthread_mutex_lock(&qe->event->mutex);
		qe->event->in_n_queues--;
		
		if (qe->event->in_n_queues == 0)
			pthread_cond_broadcast(&qe->event->cond);
		
		pthread_mutex_unlock(&qe->event->mutex);
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

void add_event(struct event_graph *graph, struct event *event) {
	struct queue_entry *eit;
	
	pthread_mutex_lock(&graph->mutex);
	
	printf("new event %s\n", event->type);
	
	for (eit=graph->equeue; eit && eit->next; eit = eit->next) {}
	
	if (eit) {
		eit->next = (struct queue_entry*) malloc(sizeof(struct queue_entry));
		eit = eit->next;
	} else {
		graph->equeue = (struct queue_entry*) malloc(sizeof(struct queue_entry));
		eit = graph->equeue;
	}
	eit->event = event;
	eit->next = 0;
	
	
	pthread_mutex_lock(&event->mutex);
	event->in_n_queues++;
	pthread_mutex_unlock(&event->mutex);
	
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

void wait_on_event(struct event *event) {
	pthread_mutex_lock(&event->mutex);
	
	while (event->in_n_queues > 0)
		pthread_cond_wait(&event->cond, &event->mutex);
	
	pthread_mutex_unlock(&event->mutex);
}

struct event *new_event() {
	struct event *event;
	int ret;
	
	event = (struct event*) calloc(1, sizeof(struct event));
	
	ret = pthread_mutex_init(&event->mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&event->cond, NULL); ASSERT(ret >= 0);
	
	return event;
}

struct event_graph *get_graph_for_event_type(char *event_type) {
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

void add_raw_event(struct event *event) {
	struct event_graph *graph;
	
	graph = get_graph_for_event_type(event->type);
	
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

void add_event_type(char *event_type) {
	n_event_types++;
	event_types = (char**) realloc(event_types, sizeof(char*)*n_event_types);
	event_types[n_event_types-1] = event_type;
}

void handle_cortexd_enable(struct event *event, void *userdata) {
	printf("cortexd enable\n");
}

struct event_task *new_task() {
	struct event_task *etask = (struct event_task*) calloc(1, sizeof(struct event_task));
	etask->position = 100;
	
	return etask;
}

void init_core() {
	unsigned int i;
	
	i=0;
	while (local_event_types[i]) {
		add_event_type(local_event_types[i]);
		i++;
	}
	
	new_eventgraph(&cortexd_graph, local_event_types);
	
	struct event_task *etask = new_task();
	etask->handle = &handle_cortexd_enable;
	
	cortexd_graph->tasks = etask;
}

void response_cache_task(struct event *event, void *userdata) {
	struct response_cache *dc = (struct response_cache*) userdata;
	size_t i;
	
	dc->cur = 0;
	
	dc->cur_key = dc->create_key(event);
	if (!dc->cur_key)
		return;
	
	for (i=0; i < dc->n_entries; i++) {
		if (!strcmp(dc->cur_key, dc->entries[i].key)) {
			event->response = dc->entries[i].response;
			event->response_size = dc->entries[i].response_size;
			
			dc->cur = &dc->entries[i];
			
			printf("response found in cache\n");
			
			return;
		}
	}
}

void response_cache_task_cleanup(struct event *event, void *userdata) {
	struct response_cache *dc = (struct response_cache*) userdata;
	
	if (dc->cur_key && !dc->cur && event->response) {
		dc->n_entries++;
		dc->entries = (struct response_cache_entry *) realloc(dc->entries, sizeof(struct response_cache_entry)*dc->n_entries);
		
		dc->entries[dc->n_entries-1].key = dc->cur_key;
		dc->entries[dc->n_entries-1].response = event->response;
		dc->entries[dc->n_entries-1].response_size = event->response_size;
	} else {
		free(dc->cur_key);
	}
}

struct event_task *create_response_cache_task(create_key_cb_t create_key) {
	struct response_cache *dc;
	struct event_task *task;
	
	task = new_task();
	
	dc = (struct response_cache*) calloc(1, sizeof(struct response_cache));
	dc->create_key = create_key;
	
	task->id = "response_cache";
	task->position = 90;
	task->userdata = dc;
	task->handle = &response_cache_task;
	task->cleanup = &response_cache_task_cleanup;
	
	return task;
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