

#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "sd_bus.h"
#include "sd_bus_notifications.h"
#include "threads.h"

// See https://developer.gnome.org/notification-spec/ for DBUS service specification

#define EV_NOTIF_SIGNAL "sd-bus.org.freedesktop.Notifications.Signal"
char *notif_event_types[] = { EV_NOTIF_SIGNAL, 0 };

static char initialized = 0;

struct wait_list_item {
	u_int32_t id;
	char *answer;
	struct crtx_signal barrier;
	
	struct wait_list_item *prev;
	struct wait_list_item *next;
};

struct wait_list_item *wait_list = 0;
pthread_mutex_t wait_list_mutex;

static sd_bus *def_bus = 0;

/// send a SD-BUS message to the notification service
static u_int32_t sd_bus_send_notification(char *icon, char *title, char *text, char **actions) {
	int r;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *reply = NULL;
	char **ait;
	int32_t id = 0;
	int32_t time = 0;
	
	if (!def_bus) {
		r = sd_bus_default(&def_bus);
		if (r < 0) {
			printf("Failed to connect to bus: %s\n", strerror(-r));
			return 0;
		}
	}
	
	r = sd_bus_message_new_method_call(def_bus,
							&m,
							"org.freedesktop.Notifications",  /* service to contact */
							"/org/freedesktop/Notifications", /* object path */
							"org.freedesktop.Notifications",  /* interface name */
							"Notify"                          /* method name */
							);
	if (r < 0) {
		fprintf(stderr, "Failed to issue method call: %s %s\n", error.name, error.message);
		return 0;
	}
	
	#define SDCHECK(r) { if (r < 0) {printf("err %d %s %d\n", r, strerror(r), __LINE__);} }
	r = sd_bus_message_append_basic(m, 's', "cortexd"); SDCHECK(r)
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
	
	
	r = sd_bus_message_append_basic(m, 'i', &time); SDCHECK(r)
	
	r = sd_bus_call(def_bus, m, -1, &error, &reply); SDCHECK(r)
	
	u_int32_t res;
	r = sd_bus_message_read(reply, "u", &res);
	if (r < 0) {
		fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
	}
	
	sd_bus_message_unref(m);
	
	return res;
}

static void notification_signal_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	sd_bus_message *m = (sd_bus_message *) event->data.raw;
	struct wait_list_item *it;
	char *s;
	const char *signal;
	uint32_t id, reason;
	
	signal = sd_bus_message_get_member(m);
	
	if (!strcmp(signal, "ActionInvoked")) {
		sd_bus_message_read_basic(m, 'u', &id);
		sd_bus_message_read_basic(m, 's', &s);
	} else
	if (!strcmp(signal, "NotificationClosed")) {
		sd_bus_message_read_basic(m, 'u', &id);
		sd_bus_message_read_basic(m, 'u', &reason);
		s = 0;
	}
	
	// lookup list of notifications and register answer
	pthread_mutex_lock(&wait_list_mutex);
	for (it=wait_list; it; it = it->next) {
		if (it->id == id) {
			if (s)
				it->answer = stracpy(s, 0);
			else
				it->answer = 0;
			
			send_signal(&it->barrier, 1);
			break;
		}
	}
	pthread_mutex_unlock(&wait_list_mutex);
}

void crtx_send_notification(char *icon, char *title, char *text, char **actions, char**chosen_action) {
	if (!initialized) {
		int ret;
		struct crtx_graph *graph;
		
		ret = pthread_mutex_init(&wait_list_mutex, 0); ASSERT(ret >= 0);
		
		graph = get_graph_for_event_type(EV_NOTIF_SIGNAL, notif_event_types);
		
		/*
		 * instruct the sd_bus module to listen for notification DBUS signals
		 */
		
		// TODO check if sd-bus module is initialized!
		
		sd_bus_add_signal_listener(sd_bus_main_bus, "/org/freedesktop/Notifications", EV_NOTIF_SIGNAL);
		
		crtx_create_task(graph, 200, "sd_bus_handle_notify_signal", &notification_signal_handler, 0);
		
		
		initialized = 1;
	}
	
	if (actions && chosen_action) {
		struct wait_list_item *it;
		
		/*
		 * register this notification as we expect a response later
		 */
		
		pthread_mutex_lock(&wait_list_mutex);
		
		// add this notification to list of unanswered notifications
		for (it = wait_list; it && it->next; it = it->next) {}
		if (it) {
			it->next = (struct wait_list_item *) malloc(sizeof(struct wait_list_item));
			it->next->prev = it;
			it = it->next;
		} else {
			wait_list = (struct wait_list_item *) malloc(sizeof(struct wait_list_item));
			it = wait_list;
			it->prev = 0;
		}
		it->next = 0;
		
		pthread_mutex_unlock(&wait_list_mutex);
		
		init_signal(&it->barrier);
		it->id = sd_bus_send_notification(icon, title, text, actions);
		
		wait_on_signal(&it->barrier);
		
		*chosen_action = it->answer;
		
		// remove this notification from list
		pthread_mutex_lock(&wait_list_mutex);
		if (it->prev)
			it->prev->next = it->next;
		else
			wait_list = it->next;
		if (it->next)
			it->next->prev = it->prev;
		pthread_mutex_unlock(&wait_list_mutex);
		
		free(it);
	} else {
		sd_bus_send_notification(icon, title, text, actions);
	}
}

/// extract necessary information from the cortex dict to call send_notification
static void notify_send_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
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
	if (ret && actions_dict) {
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
	
	crtx_send_notification("", title, msg, actions, &answer);
	
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
	}
	
	event->response.dict = data;
}

struct crtx_listener_base *crtx_new_sd_bus_notification_listener(void *options) {
	struct crtx_sd_bus_notification_listener *slistener;
	struct crtx_graph *global_notify_graph;
	
	// TODO: convert this into a module as we likely need only one listener?
	
	slistener = (struct crtx_sd_bus_notification_listener *) options;
	slistener->parent.graph = get_graph_for_event_type(EV_NOTIF_SIGNAL, notif_event_types);
	
	/*
	 * add this listener to the global notification graph
	 */
	
	global_notify_graph = get_graph_for_event_type(CRTX_EVT_NOTIFICATION, crtx_evt_notification);
	
	crtx_create_task(global_notify_graph, 0, "sd_bus_notify_send", &notify_send_handler, slistener);
	
	return &slistener->parent;
}

void crtx_sdbus_notification_init() {}

void crtx_sdbus_notification_finish() {
	if (def_bus) {
		sd_bus_flush(def_bus);
		sd_bus_unref(def_bus);
// 		sd_bus_close(def_bus);
	}
}