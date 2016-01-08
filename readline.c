
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "core.h"
#include "readline.h"

static char *event_types[] = { "readline/request", 0 };

pthread_mutex_t stdout_mutex;


/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char * readline_read() {
	char *line_read = 0;
	
	line_read = readline ("> ");
	
	if (line_read && *line_read)
		add_history (line_read);
	
	return line_read;
}

void *readline_tmain(void *data) {
	
	return 0;
}

void handle_request(struct event *event, void *userdata, void **sessiondata) {
	char *input;
	
	pthread_mutex_lock(&stdout_mutex);
	
	printf("event: %s\n", (char*) event->data.raw);
	
	input = readline_read();
	
	event->response.raw = input;
	
	pthread_mutex_unlock(&stdout_mutex);
}

void readline_init() {
	struct event_graph *event_graph;
	int ret;
	
	ret = pthread_mutex_init(&stdout_mutex, 0); ASSERT(ret >= 0);
	
	new_eventgraph(&event_graph, event_types);
	
	struct event_task *etask = new_task();
	etask->handle = &handle_request;
	event_graph->tasks = etask;
}

void readline_finish() {
}