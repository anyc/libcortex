
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "core.h"
#include "readline.h"

static char *event_types[] = { "readline/request", 0 };

pthread_mutex_t stdout_mutex;

// static pthread_t thread;
// struct readline_thread_args sargs;

// struct readline_thread_args {
// 	struct event_graph *graph;
// 	char stop;
// };


/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char * readline_read() {
// 	static char *line_read = (char *)NULL;
	char *line_read = 0;
	
// 	if (line_read) {
// 		free(line_read);
// 		line_read = (char *)NULL;
// 	}
	
// 	printf("readline %p\n", line_read);
	
	line_read = readline ("> ");
	
	if (line_read && *line_read)
		add_history (line_read);
	
	return line_read;
}

void *readline_tmain(void *data) {
	
	return 0;
}

void handle_request(struct event *event, void *userdata) {
	char *input;
	
	pthread_mutex_lock(&stdout_mutex);
	
	printf("event: %s\n", (char*) event->data);
	
	input = readline_read();
	
	event->response = input;
	
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
	
// 	sargs.graph = event_graph;
// 	sargs.stop = 0;
	
// 	pthread_create(&thread, NULL, readline_tmain, &sargs);
}

void readline_finish() {
}