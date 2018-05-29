/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "intern.h"
#include "core.h"
#include "readline.h"
#include "threads.h"
#include "dict.h"

// #define EV_RL_REQUEST "cortex.readline.request"
// static char *rl_event_types[] = { EV_RL_REQUEST, 0 };

static pthread_mutex_t stdout_mutex;


/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
static char * readline_read() {
	char *line_read = 0;
	
	line_read = readline ("> ");
	
	if (line_read && *line_read)
		add_history (line_read);
	
	return line_read;
}

void crtx_readline_show_notification(char *title, char *text, char **actions, char**chosen_action) {
	char *input;
	char **a;
	
	pthread_mutex_lock(&stdout_mutex);
	
	INFO("Notification \"%s\": \"%s\"\n", title, text);
	if (actions) {
		INFO("Possible actions:\n");
		a = actions;
		while (*a) {
			INFO("\t\"%s\" - type \"%s\"\n", *a, *(a+1) );
			a += 2;
		}
		
		INFO("Response: ");
		input = readline_read();
		
		*chosen_action = input;
	}
	
	pthread_mutex_unlock(&stdout_mutex);
}

/// extract necessary information from the cortex dict to call send_notification
static char rl_notify_send_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	char *title, *msg, *answer;
	char ret;
	size_t answer_length;
	struct crtx_dict *data, *actions_dict;
	char **actions;
	struct crtx_dict *dict;
	
	crtx_event_get_payload(event, 0, 0, &dict);
	
	ret = crtx_get_value(dict, "title", 's', &title, sizeof(void*));
	if (!ret) {
		printf("error parsing event\n");
		return 1;
	}
	
	ret = crtx_get_value(dict, "message", 's', &msg, sizeof(void*));
	if (!ret) {
		printf("error parsing event\n");
		return 1;
	}
	
	ret = crtx_get_value(dict, "actions", 'D', &actions_dict, sizeof(void*));
	if (ret && actions_dict) {
		struct crtx_dict_item *di;
		unsigned char i;
		
		actions = (char**) calloc(1, sizeof(char*)*(actions_dict->signature_length+1));
		
		di = crtx_get_first_item(actions_dict);
		if (!di) {
			printf("error parsing event\n");
			free(actions);
			return 1;
		}
		i = 0;
		
		while (di && i < 4) {
			ret = crtx_copy_value(di, &actions[i], sizeof(void*));
			if (!ret) {
				printf("error parsing event\n");
				free(actions);
				return 1;
			}
			
			di = crtx_get_next_item(actions_dict, di);
			i++;
		}
		actions[4] = 0;
	} else
		actions = 0;
	
	crtx_readline_show_notification(title, msg, actions, &answer);
	
	data = 0;
	if (actions) {
		if (answer) {
			answer_length = strlen(answer);
			data = crtx_create_dict("s",
							    "action", answer, answer_length, 0
			);
		} else {
			data = crtx_create_dict("s",
							    "action", 0, 0, 0
			);
		}
		free(actions);
	}
	
	event->response.dict = data;
	
	return 1;
}

// struct crtx_listener_base *crtx_new_readline_listener(void *options) {
// 	struct crtx_readline_listener *slistener;
// 	struct crtx_graph *global_notify_graph;
// 	
// 	slistener = (struct crtx_readline_listener *) options;
// 	slistener->base.graph = get_graph_for_event_type(EV_RL_REQUEST, rl_event_types);
// 	
// 	/*
// 	 * add this listener to the global notification graph
// 	 */
// 	
// 	global_notify_graph = get_graph_for_event_type(CRTX_EVT_NOTIFICATION, crtx_evt_notification);
// 	
// 	crtx_create_task(global_notify_graph, 0, "rl_notify_send_handler", &rl_notify_send_handler, slistener);
// 	
// 	return &slistener->base;
// }

void crtx_readline_init() {
	int ret;
	
	crtx_register_handler_for_event_type("cortex.user_notification", "readline_notify", &rl_notify_send_handler, 0);
	
	ret = pthread_mutex_init(&stdout_mutex, 0); ASSERT(ret >= 0);
}

void crtx_readline_finish() {
	int ret;
	
	ret = pthread_mutex_destroy(&stdout_mutex); ASSERT(ret >= 0);
}
