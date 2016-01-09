
#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "sd_bus.h"
#include "sd_bus_notifications.h"

#define EV_NOTIF_AC_INVOKED "sd-bus.org.freedesktop.Notifications.ActionInvoked"
char *notif_event_types[] = { EV_NOTIF_AC_INVOKED, 0 };

static char initialized = 0;
static struct crtx_graph *graph;

struct sd_bus_notifications_item {
	u_int32_t id;
	char *answer;
	struct crtx_signal barrier;
	
	struct sd_bus_notifications_item *prev;
	struct sd_bus_notifications_item *next;
};

// static u_int32_t *notif_ids = 0;
// static size_t n_notif_ids = 0;
// static struct crtx_signal *signals = 0;
// static char **answers = 0;
struct sd_bus_notifications_item *wait_list = 0;
pthread_mutex_t notif_mutex;




static u_int32_t sd_bus_send_notification(char *icon, char *title, char *text, char **actions) {
	int r;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *reply = NULL;
	char **ait;
	sd_bus *bus;
	
	r = sd_bus_default(&bus);
	if (r < 0) {
		printf("Failed to connect to bus: %s\n", strerror(-r));
		return 0;
	}
	
	// 	r = sd_bus_message_new_method_call(bus, &m, dest, path, interface, member);
	r = sd_bus_message_new_method_call(bus,
								&m,
							 "org.freedesktop.Notifications",           /* service to contact */
							 "/org/freedesktop/Notifications",          /* object path */
							 "org.freedesktop.Notifications",   /* interface name */
							 "Notify"                         /* method name */
	);
	if (r < 0) {
		fprintf(stderr, "Failed to issue method call: %s %s\n", error.name, error.message);
		return 0;
	}
	
	#define SDCHECK(r) { if (r < 0) {printf("err %d %s %d\n", r, strerror(r), __LINE__);} }
	r = sd_bus_message_append_basic(m, 's', "cortexd"); SDCHECK(r)
	int32_t id = 0;
	r = sd_bus_message_append_basic(m, 'u', &id); SDCHECK(r)
	r = sd_bus_message_append_basic(m, 's', icon); SDCHECK(r)
	r = sd_bus_message_append_basic(m, 's', title); SDCHECK(r)
	r = sd_bus_message_append_basic(m, 's', text); SDCHECK(r)
	
	r = sd_bus_message_open_container(m, 'a', "s"); SDCHECK(r)
	if (actions) {
		ait = actions;
		while (*ait) {
			r = sd_bus_message_append_basic(m, 's', *ait); SDCHECK(r)
			ait++;
		}
	} else {
		r = sd_bus_message_append_basic(m, 's', ""); SDCHECK(r)
	}
	r = sd_bus_message_close_container(m); SDCHECK(r)
	
	r = sd_bus_message_open_container(m, 'a', "{sv}"); SDCHECK(r)
	r = sd_bus_message_open_container(m, 'e', "sv"); SDCHECK(r)
	r = sd_bus_message_append_basic(m, 's', ""); SDCHECK(r)
	
	r = sd_bus_message_open_container(m, 'v', "s"); SDCHECK(r)
	r = sd_bus_message_append_basic(m, 's', ""); SDCHECK(r)
	r = sd_bus_message_close_container(m); SDCHECK(r)
	r = sd_bus_message_close_container(m); SDCHECK(r)
	r = sd_bus_message_close_container(m); SDCHECK(r)
	
	int32_t time = 0;
	r = sd_bus_message_append_basic(m, 'i', &time); SDCHECK(r)
	
	r = sd_bus_call(bus, m, -1, &error, &reply); SDCHECK(r)
	
	u_int32_t res;
	r = sd_bus_message_read(reply, "u", &res);
	if (r < 0) {
		fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
	}
	
	return res;
}

void handle_notification_response(struct crtx_event *event, void *userdata, void **sessiondata) {
	sd_bus_message *m = (sd_bus_message *) event->data.raw;
	struct sd_bus_notifications_item *it;
	char *s;
	uint32_t uint32;
	
	sd_bus_message_read_basic(m, 'u', &uint32);
	sd_bus_message_read_basic(m, 's', &s);
	
	pthread_mutex_lock(&notif_mutex);
	for (it=wait_list; it; it = it->next) {
		if (it->id == uint32) {
			it->answer = stracpy(s, 0);
			send_signal(&it->barrier, 1);
			break;
		}
	}
// 	for (i=0; i<n_notif_ids; i++) {
// 		if (notif_ids[i] == uint32) {
// 			answers[i] = s;
// 			send_signal(&signals[i], 1);
// 			break;
// 		}
// 	}
	pthread_mutex_unlock(&notif_mutex);
}


void send_notification(char *icon, char *title, char *text, char **actions, char**chosen_action) {
	if (!initialized) {
		int ret;
		printf("initialize\n");
		ret = pthread_mutex_init(&notif_mutex, 0); ASSERT(ret >= 0);
		
// 		graph = get_graph_for_event_type(EV_NOTIF_AC_INVOKED);
// 		if (!graph) {
// 			new_eventgraph(&graph, notif_event_types);
// 		}
		graph = get_graph_for_event_type(EV_NOTIF_AC_INVOKED, notif_event_types);
		
		sd_bus_add_listener(sd_bus_main_bus, "/org/freedesktop/Notifications", EV_NOTIF_AC_INVOKED);
		
		struct crtx_task *task2;
		task2 = new_task();
		task2->id = "notif_handler";
		task2->handle = &handle_notification_response;
		task2->userdata = 0;
		add_task(graph, task2);
		
		initialized = 1;
	}
	
	if (actions) {
// 		unsigned int id;
		struct sd_bus_notifications_item *it;
		
		pthread_mutex_lock(&notif_mutex);
		
// 		n_notif_ids++;
// 		notif_ids = (u_int32_t*) realloc(notif_ids, sizeof(u_int32_t)*n_notif_ids);
// 		signals = (struct crtx_signal*) realloc(signals, sizeof(struct crtx_signal)*n_notif_ids);
// 		answers = (char **) realloc(answers, sizeof(char*)*n_notif_ids);
		for (it = wait_list; it && it->next; it = it->next) {}
		if (it) {
			it->next = (struct sd_bus_notifications_item *) malloc(sizeof(struct sd_bus_notifications_item));
			it->next->prev = it;
			it = it->next;
		} else {
			wait_list = (struct sd_bus_notifications_item *) malloc(sizeof(struct sd_bus_notifications_item));
			it = wait_list;
			it->prev = 0;
		}
		it->next = 0;
		pthread_mutex_unlock(&notif_mutex);
		
// 		init_signal(&signals[n_notif_ids-1]);
		init_signal(&it->barrier);
		
// 		id = n_notif_ids-1;
// 		notif_ids[id] = 0;
		
		it->id = sd_bus_send_notification(icon, title, text, actions);
		
		printf("notification id %u\n", it->id);
		
		wait_on_signal(&it->barrier);
		
		*chosen_action = it->answer;
		
		pthread_mutex_lock(&notif_mutex);
		if (it->prev)
			it->prev->next = it->next;
		else
			wait_list = it->next;
		if (it->next)
			it->next->prev = it->prev;
		pthread_mutex_unlock(&notif_mutex);
		
		free(it);
	} else {
		sd_bus_send_notification(icon, title, text, actions);
	}
}

// char *test[5] = { "yes", "YES", "no", "NO", 0 };
static void notify_task_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	char *title, *msg, *answer;
	char ret;
	size_t answer_length;
	struct crtx_dict *data, *actions_dict;
	char **actions;
	
	ret = crtx_get_value(event->data.dict, "title", &title, sizeof(void*));
	if (!ret) {
		printf("error parsing event\n");
		return;
	}
	
	ret = crtx_get_value(event->data.dict, "message", &msg, sizeof(void*));
	if (!ret) {
		printf("error parsing event\n");
		return;
	}
	
	ret = crtx_get_value(event->data.dict, "actions", &actions_dict, sizeof(void*));
	if (ret) {
		struct crtx_dict_item *di;
		unsigned char i;
		
		actions = (char**) calloc(1, sizeof(char*)*(actions_dict->signature_length+1));
		
		di = crtx_get_first_item(actions_dict);
		if (!di) {
			printf("error parsing event\n");
			free(actions);
			return;
		}
		i = 0;
		
		while (di && i < 4) {
			ret = crtx_copy_value(di, &actions[i], sizeof(void*));
			if (!ret) {
				printf("error parsing event\n");
				free(actions);
				return;
			}
			
			di = crtx_get_next_item(di);
			i++;
		}
		actions[4] = 0;
	} else
		actions = 0;
	
	send_notification("", title, msg, actions, &answer);
	
	answer_length = strlen(answer);
	data = crtx_create_dict("s",
				"action", answer, answer_length, 0
				);
	
// 	event->response = data;
// 	response = create_response(event, 0, 0);
	
// 	memset(&ed, 0, sizeof(struct crtx_event_data));
// 	ed.dict = data;
	
// 	if (event->respond)
// 		event->respond(event, &ed);
// 	else
	event->response.dict = data;
	
// 	return_event(response);
	
// 	printf("ans %s\n", answer);
}

struct crtx_listener_base *new_sd_bus_notification_listener(void *options) {
	struct sd_bus_notifications_listener *slistener;
	struct crtx_graph *graph;
	
	slistener = (struct sd_bus_notifications_listener *) options;
	
	slistener->parent.graph = get_graph_for_event_type(EV_NOTIF_AC_INVOKED, notif_event_types);
	
	graph = get_graph_for_event_type(CRTX_EVT_NOTIFICATION, crtx_evt_notification);
	
	struct crtx_task * notify_task;
	notify_task = new_task();
	notify_task->id = "notify_task";
	notify_task->handle = &notify_task_handler;
	notify_task->userdata = slistener;
	add_task(graph, notify_task);
	
	return &slistener->parent;
}
