#include <stdio.h>
#include <stdlib.h>

#include "core.h"

char ** event_types = 0;
unsigned n_event_types = 0;

struct event_graph *cortexd_graph;

struct event_graph **graphs;
unsigned int n_graphs;

static char * local_event_types[] = { "cortexd_enable", 0 };

#include "socket.h"
#ifdef STATIC_SD_BUS
#include "sd_bus.h"
#endif

struct plugin static_plugins[] = {
	{"socket", &socket_init, &socket_finish},
#ifdef STATIC_SD_BUS
	{"sd-bus", &sd_bus_init, &sd_bus_finish},
#endif
	{0, 0}
};

void *graph_consumer_main(void *arg) {
	struct thread *self = (struct thread*) arg;
	struct event *e;
	
	while (!self->stop) {
		pthread_mutex_lock(&self->graph->queue_mutex);
		
		while (self->graph->n_queue_entries == 0)
			pthread_cond_wait(&self->graph->queue_cond, &self->graph->queue_mutex);
		
		e = self->graph->equeue;
		self->graph->equeue = self->graph->equeue->next;
		self->graph->n_queue_entries--;
		
		self->graph->entry_task->handle(e, self->graph->entry_task->userdata);
		
		free(e);
		
		pthread_mutex_unlock(&self->graph->queue_mutex);
	}
}

void spawn_thread(struct event_graph *graph) {
// 	pthread_mutex_lock(&graph->mutex);
	
	graph->n_consumers++;
	graph->consumers = (struct thread*) realloc(graph->consumers, sizeof(struct thread)*graph->n_consumers); ASSERT(graph->consumers);
	
	graph->consumers[graph->n_consumers-1].stop = 0;
	graph->consumers[graph->n_consumers-1].graph = graph;
	pthread_create(&graph->consumers[graph->n_consumers-1].handle, NULL, &graph_consumer_main, &graph->consumers[graph->n_consumers-1]);
	
// 	pthread_mutex_unlock(&graph->mutex);
}

void add_event(struct event_graph *graph, struct event *event) {
	struct event *eit;
	
	pthread_mutex_lock(&graph->mutex);
	
	printf("new event %s\n", event->type);
	
	for (eit=graph->equeue; eit && eit->next; eit = eit->next) {}
	
	if (eit) {
		eit->next = event;
	} else {
		graph->equeue = event;
	}
	event->next = 0;
	graph->n_queue_entries++;
	
	if (graph->n_consumers == 0)
		spawn_thread(graph);
	
	pthread_cond_signal(&graph->queue_cond);
	
	pthread_mutex_unlock(&graph->mutex);
}

void add_raw_event(struct event *event) {
	struct event_graph *graph;
	unsigned int i, j;
	
	graph = 0;
	for (i=0; i < n_graphs && !graph; i++) {
		for (j=0; j < graphs[i]->n_types; j++) {printf("%s %s\n", graphs[i]->types[j], event->type);
			if (!strcmp(graphs[i]->types[j], event->type)) {
				graph = graphs[i];
				break;
			}
		}
	}
	
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

void init_core() {
	unsigned int i;
	
	i=0;
	while (local_event_types[i]) {
		add_event_type(local_event_types[i]);
		i++;
	}
	
	new_eventgraph(&cortexd_graph, local_event_types);
	
	struct event_task *etask = (struct event_task*) calloc(1, sizeof(struct event_task));
	etask->handle = &handle_cortexd_enable;
	
	cortexd_graph->entry_task = etask;
}


