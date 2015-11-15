#include <stdio.h>
#include <stdlib.h>

#include "core.h"

char ** event_types = 0;
unsigned n_event_types = 0;

#include "socket.h"
#ifdef STATIC_SD_BUS
#include "sd_bus.h"
#endif

struct plugin static_plugins[] = {
	{"socket", &socket_init},
#ifdef STATIC_SD_BUS
	{"sd-bus", &sd_bus_init},
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
		
		self->graph->entry_task->handle(e);
		
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

void new_eventgraph(struct event_graph **event_graph, char **event_types) {
	struct event_graph *graph;
	int ret;
	
	graph = (struct event_graph*) calloc(1, sizeof(struct event_graph)); ASSERT(graph);
	
	*event_graph = graph;
	
	ret = pthread_mutex_init(&graph->mutex, 0); ASSERT(ret >= 0);
	
	ret = pthread_mutex_init(&graph->queue_mutex, 0); ASSERT(ret >= 0);
	ret = pthread_cond_init(&graph->queue_cond, NULL); ASSERT(ret >= 0);
	
	while (event_types[graph->n_types])
		graph->n_types++;
	
	graph->types = event_types;
}

void add_event_type(char *event_type) {
	n_event_types++;
	event_types = (char**) realloc(event_types, sizeof(char*)*n_event_types);
	event_types[n_event_types-1] = event_type;
}





