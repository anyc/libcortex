
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "core.h"
#include "readline.h"
#include "threads.h"

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

void handle_request(struct crtx_event *event, void *userdata, void **sessiondata) {
	char *input;
	
	pthread_mutex_lock(&stdout_mutex);
	
	printf("event: %s\n", (char*) event->data.raw);
	
	input = readline_read();
	
	event->response.raw = input;
	
	pthread_mutex_unlock(&stdout_mutex);
}

void crtx_readline_init() {
	struct crtx_graph *crtx_graph;
	int ret;
	
	ret = pthread_mutex_init(&stdout_mutex, 0); ASSERT(ret >= 0);
	
	new_eventgraph(&crtx_graph, event_types);
	
	struct crtx_task *etask = new_task();
	etask->handle = &handle_request;
	crtx_graph->tasks = etask;
}

void crtx_readline_finish() {
}