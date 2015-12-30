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
	
	if (ti->handle)
		ti->handle(event, ti->userdata, &sessiondata);
	
	printf("execute task %s\n", ti->id);
	
	if (!event->response && ti->next)
		traverse_graph_r(ti->next, event);
	
	if (ti->cleanup)
		ti->cleanup(event, ti->userdata, sessiondata);
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
		
		traverse_graph_r(self->graph->tasks, qe->event);
		
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

void free_event(struct event *event) {
	free(event);
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

char rcache_match_cb_t_strcmp(struct response_cache *rc, struct event *event, struct response_cache_entry *c_entry) {
	return !strcmp( (char*) rc->cur_key, (char*) c_entry->key);
}

char rcache_match_cb_t_regex(struct response_cache *rc, struct event *event, struct response_cache_entry *c_entry) {
	int ret;
	char msgbuf[128];
	
	if (!c_entry->regex_initialized) {
		ret = regcomp(&c_entry->key_regex, (char*) c_entry->key, 0);
		if (ret) {
			fprintf(stderr, "Could not compile regex %s\n", (char*) c_entry->key);
			exit(1);
		}
		c_entry->regex_initialized = 1;
	}
	
	ret = regexec( &c_entry->key_regex, (char *) rc->cur_key, 0, NULL, 0);
	if (!ret) {
		return 1;
	} else if (ret == REG_NOMATCH) {
		return 0;
	} else {
		regerror(ret, &c_entry->key_regex, msgbuf, sizeof(msgbuf));
		fprintf(stderr, "Regex match failed: %s\n", msgbuf);
		exit(1);
	}
}

void response_cache_task(struct event *event, void *userdata, void **sessiondata) {
	struct response_cache *dc = (struct response_cache*) userdata;
	size_t i;
	
// 	dc->cur = 0;
	
	dc->cur_key = dc->create_key(event);
	if (!dc->cur_key)
		return;
	
	for (i=0; i < dc->n_entries; i++) {
		if (dc->match_event(dc, event, &dc->entries[i])) {
			event->response = dc->entries[i].response;
			event->response_size = dc->entries[i].response_size;
			
			*sessiondata = &dc->entries[i];
			
			printf("response found in cache: ");
			if (dc->signature[0] == 's')
				printf("key %s ", (char*) dc->entries[i].key);
			printf("\n");
			
			return;
		}
	}
}

void response_cache_task_cleanup(struct event *event, void *userdata, void *sessiondata) {
	struct response_cache *dc = (struct response_cache*) userdata;
	
	if (dc->cur_key && !sessiondata && event->response) {
		dc->n_entries++;
		dc->entries = (struct response_cache_entry *) realloc(dc->entries, sizeof(struct response_cache_entry)*dc->n_entries);
		
		memset(&dc->entries[dc->n_entries-1], 0, sizeof(struct response_cache_entry));
		dc->entries[dc->n_entries-1].key = dc->cur_key;
		dc->entries[dc->n_entries-1].response = event->response;
		dc->entries[dc->n_entries-1].response_size = event->response_size;
	} else {
		free(dc->cur_key);
	}
}

// char * crtx_get_trim_copy(char * str, unsigned int maxlen) {
// 	char * s, *ret;
// 	unsigned int i, len;
// 	
// 	if (!str || maxlen == 0)
// 		return 0;
// 	
// 	i=0;
// 	// get start and reduce maxlen
// 	while (isspace(*str) && i < maxlen) {str++;i++;}
// 	maxlen -= i;
// 	
// 	// goto end, and move backwards
// 	s = str + (strlen(str) < maxlen? strlen(str) : maxlen) - 1;
// 	while (s > str && isspace(*s)) {s--;}
// 	
// 	len = (s-str + 1 < maxlen?s-str + 1:maxlen);
// 	if (len == 0)
// 		return 0;
// 	
// 	ret = (char*) dls_malloc( len + 1 );
// 	strncpy(ret, str, len);
// 	ret[len] = 0;
// 	
// 	return ret;
// }

void prefill_rcache(struct response_cache *rcache, char *file) {
	FILE *f;
	char buf[1024];
	char *indent;
	size_t slen;
	
// 	if (strcmp(rcache->signature, "ss")) {
// 		printf("rcache signature %s != ss\n", rcache->signature);
// 		return;
// 	}
	
	f = fopen(file, "r");
	
	if (!f)
		return;
	
	while (fgets(buf, sizeof(buf), f)) {
		if (buf[0] == '#')
			continue;
		
		indent = strchr(buf, '\t');
		
		if (!indent)
			continue;
		
		rcache->n_entries++;
		rcache->entries = (struct response_cache_entry *) realloc(rcache->entries, sizeof(struct response_cache_entry)*rcache->n_entries);
		memset(&rcache->entries[rcache->n_entries-1], 0, sizeof(struct response_cache_entry));
		
		slen = indent - buf;
		rcache->entries[rcache->n_entries-1].key = stracpy(buf, &slen);
// 		printf("asd %s\n", (char*)rcache->entries[rcache->n_entries-1].key);
		if (rcache->signature[1] == 's') {
			slen = strlen(buf) - slen - 1;
			if (buf[strlen(buf)-1] == '\n')
				slen--;
			rcache->entries[rcache->n_entries-1].response = stracpy(indent+1, &slen);;
			rcache->entries[rcache->n_entries-1].response_size = slen;
// 			printf("asd %s\n", (char*)rcache->entries[rcache->n_entries-1].response);
		} else
		if (rcache->signature[1] == 'u') {
			uint32_t uint;
			rcache->entries[rcache->n_entries-1].response_size = strlen(buf) - slen - 2;
			
			sscanf(indent+1, "%"PRIu32,  &uint);
// 			printf("%"PRIu32"\n", uint);
			rcache->entries[rcache->n_entries-1].response = (void*)(ptrdiff_t) uint;
		}
	}
	
	if (ferror(stdin)) {
		fprintf(stderr,"Oops, error reading stdin\n");
		exit(1);
	}
	
	fclose(f);
}


struct event_task *create_response_cache_task(char *signature, create_key_cb_t create_key) {
	struct response_cache *dc;
	struct event_task *task;
	
	task = new_task();
	
	dc = (struct response_cache*) calloc(1, sizeof(struct response_cache));
	dc->signature = signature;
	dc->create_key = create_key;
	dc->match_event = &rcache_match_cb_t_strcmp;
	
	task->id = "response_cache";
	task->position = 90;
	task->userdata = dc;
	task->handle = &response_cache_task;
	task->cleanup = &response_cache_task_cleanup;
	
	return task;
}

void free_response_cache(struct response_cache *dc) {
	size_t i;
	
	for (i=0; i<dc->n_entries; i++) {
		free(dc->entries[i].key);
		if (dc->entries[i].response_size > 0)
			free(dc->entries[i].response);
		
		if (dc->entries[i].regex_initialized)
			regfree(&dc->entries[i].key_regex);
	}
	
	free(dc->entries);
	free(dc);
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

void free_data_struct(struct data_struct *ds) {
	struct data_item *di;
	
	if (!ds)
		return;
	
	di = ds->items;
	while (1) {
		if (di->flags & DIF_KEY_ALLOCATED)
			free(di->key);
		
		switch (di->type) {
			case 's':
				if (!(di->flags & DIF_DATA_UNALLOCATED))
					free(di->string);
				break;
			case 'D':
				free_data_struct(di->ds);
				break;
		}
		
		if (DIF_IS_LAST(di))
			break;
		
		di++;
	}
	
	free(ds);
}