/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "intern.h"
#include "core.h"
#include "sdbus.h"
#include "sdbus_notifications.h"
#include "threads.h"
#include "dict.h"

// See https://developer.gnome.org/notification-spec/ for DBUS service specification

#define EV_NOTIF_SIGNAL "sd-bus.org.freedesktop.Notifications.Signal"
char *notif_event_types[] = { EV_NOTIF_SIGNAL, 0 };

struct wait_list_item {
	struct crtx_dll ll;
	
	uint32_t id;
	char *answer;
	struct crtx_signals barrier;
};

struct crtx_sdbus_notifications {
	char initialized;

	struct wait_list_item *wait_list;
	pthread_mutex_t wait_list_mutex;
	
	struct crtx_sdbus_listener *sdbus_lstnr;
};

static struct crtx_sdbus_notifications *notif_root = 0;

static struct crtx_sdbus_match notification_match[] = {
	/* slot, match string, event type, callback fct, callback data, <reserved> */
	{ 0, "type='signal',path='/org/freedesktop/Notifications'", EV_NOTIF_SIGNAL, 0, 0, 0 },
	{0},
};

static int notification_signal_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	sd_bus_message *m = (sd_bus_message *) event->data.pointer;
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
	pthread_mutex_lock(&notif_root->wait_list_mutex);
	for (it=notif_root->wait_list; it; it = (struct wait_list_item *) it->ll.next) {
		if (it->id == id) {
			if (s)
				it->answer = crtx_stracpy(s, 0);
			else
				it->answer = 0;
			
			crtx_send_signal(&it->barrier, 1);
			break;
		}
	}
	pthread_mutex_unlock(&notif_root->wait_list_mutex);
	
	return 1;
}

static void init_listener() {
	if (!notif_root->initialized) {
		int ret;
		struct crtx_graph *graph;
		
		
		ret = pthread_mutex_init(&notif_root->wait_list_mutex, 0); CRTX_ASSERT(ret >= 0);
		graph = crtx_get_graph_for_event_description(EV_NOTIF_SIGNAL, notif_event_types);
		
		/*
		 * instruct the sd_bus module to listen for notification DBUS signals
		 */
		
		// TODO check if sd-bus module is initialized!
		
		notif_root->sdbus_lstnr = crtx_sdbus_get_default_listener(CRTX_SDBUS_TYPE_USER);
		
		crtx_sdbus_match_add(notif_root->sdbus_lstnr, &notification_match[0]);
		
		crtx_create_task(graph, 200, "sd_bus_handle_notify_signal", &notification_signal_handler, 0);
		
		notif_root->initialized = 1;
	}
}

/// send a SD-BUS message to the notification service
static uint32_t sdbus_send_notification(char *icon, char *title, char *text, char **actions) {
	int r;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *reply = NULL;
	char **ait;
	int32_t id = 0;
	int32_t time = 0;
	
// 	if (!def_bus) {
// 		r = sd_bus_default(&def_bus);
// 		if (r < 0) {
// 			printf("Failed to connect to bus: %s\n", strerror(-r));
// 			return 0;
// 		}
// 	}
	init_listener();
	
	r = sd_bus_message_new_method_call(notif_root->sdbus_lstnr->bus,
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
	
	r = sd_bus_message_append_basic(m, 's', "cortexd"); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 'u', &id); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', icon); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', title); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', text); CRTX_RET_GEZ(r)
	
	r = sd_bus_message_open_container(m, 'a', "s"); CRTX_RET_GEZ(r)
		if (actions) {
			ait = actions;
			while (*ait) {
				r = sd_bus_message_append_basic(m, 's', *ait); CRTX_RET_GEZ(r)
				ait++;
			}
		} else {
			r = sd_bus_message_append_basic(m, 's', ""); CRTX_RET_GEZ(r)
		}
	r = sd_bus_message_close_container(m); CRTX_RET_GEZ(r)
	
	
	r = sd_bus_message_open_container(m, 'a', "{sv}"); CRTX_RET_GEZ(r)
		r = sd_bus_message_open_container(m, 'e', "sv"); CRTX_RET_GEZ(r)
			r = sd_bus_message_append_basic(m, 's', ""); CRTX_RET_GEZ(r)
			
			r = sd_bus_message_open_container(m, 'v', "s"); CRTX_RET_GEZ(r)
				r = sd_bus_message_append_basic(m, 's', ""); CRTX_RET_GEZ(r)
			r = sd_bus_message_close_container(m); CRTX_RET_GEZ(r)
		r = sd_bus_message_close_container(m); CRTX_RET_GEZ(r)
	r = sd_bus_message_close_container(m); CRTX_RET_GEZ(r)
	
	
	r = sd_bus_message_append_basic(m, 'i', &time); CRTX_RET_GEZ(r)
	
	r = sd_bus_call(notif_root->sdbus_lstnr->bus, m, -1, &error, &reply); CRTX_RET_GEZ(r)
	
	uint32_t res;
	r = sd_bus_message_read(reply, "u", &res);
	if (r < 0) {
		fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
	}
	
	sd_bus_message_unref(m);
	
	return res;
}

void crtx_send_notification(char *icon, char *title, char *text, char **actions, char**chosen_action) {
	init_listener();
	
	if (actions && chosen_action) {
		struct wait_list_item *it;
		
		/*
		 * register this notification as we expect a response later
		 */
		
		pthread_mutex_lock(&notif_root->wait_list_mutex);
		
		// add this notification to list of unanswered notifications
// 		for (it = wait_list; it && it->next; it = it->next) {}
// 		if (it) {
// 			it->next = (struct wait_list_item *) malloc(sizeof(struct wait_list_item));
// 			it->next->prev = it;
// 			it = it->next;
// 		} else {
// 			wait_list = (struct wait_list_item *) malloc(sizeof(struct wait_list_item));
// 			it = wait_list;
// 			it->prev = 0;
// 		}
// 		it->next = 0;
		
		it = (struct wait_list_item *) calloc(1, sizeof(struct wait_list_item));
		
		crtx_dll_append((struct crtx_dll**) &notif_root->wait_list, &it->ll);
		
		pthread_mutex_unlock(&notif_root->wait_list_mutex);
		
		crtx_init_signal(&it->barrier);
		it->id = sdbus_send_notification(icon, title, text, actions);
		
		crtx_wait_on_signal(&it->barrier, 0);
		
		*chosen_action = it->answer;
		
		// remove this notification from list
		pthread_mutex_lock(&notif_root->wait_list_mutex);
// 		if (it->prev)
// 			it->prev->next = it->next;
// 		else
// 			wait_list = it->next;
// 		if (it->next)
// 			it->next->prev = it->prev;
		crtx_dll_unlink((struct crtx_dll**) &notif_root->wait_list, &it->ll);
		
		pthread_mutex_unlock(&notif_root->wait_list_mutex);
		
		free(it);
	} else {
		sdbus_send_notification(icon, title, text, 0);
	}
}

#if 0
/// extract necessary information from the cortex dict to call send_notification
static int notify_send_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	char *title, *msg, *answer, *icon;
	char ret;
	size_t answer_length;
	struct crtx_dict *data, *actions_dict;
	char **actions;
	struct crtx_dict *dict;
	
	
	crtx_event_get_payload(event, 0, 0, &dict);
	
	title = msg = icon = 0;
	crtx_get_value(dict, "title", 's', &title, sizeof(void*));
	crtx_get_value(dict, "message", 's', &msg, sizeof(void*));
	crtx_get_value(dict, "icon", 's', &icon, sizeof(void*));
	
	if (!title && !msg) {
		CRTX_ERROR("error, no title and no message in dict\n");
		crtx_print_dict(event->data.dict);
		return 1;
	}
	
	actions = 0;
	ret = crtx_get_value(event->data.dict, "actions", 'D', &actions_dict, sizeof(void*));
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
	
	crtx_send_notification(icon, title, msg, actions, &answer);
	
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
#endif

// struct crtx_listener_base *crtx_setup_sd_bus_notification_listener(void *options) {
// 	struct crtx_sd_bus_notification_listener *slistener;
// 	struct crtx_graph *global_notify_graph;
// 	
// 	// TODO: convert this into a module as we likely need only one listener?
// 	
// 	slistener = (struct crtx_sd_bus_notification_listener *) options;
// 	slistener->base.graph = get_graph_for_event_type(EV_NOTIF_SIGNAL, notif_event_types);
// 	
// 	/*
// 	 * add this listener to the global notification graph
// 	 */
// 	
// 	global_notify_graph = get_graph_for_event_type(CRTX_EVT_NOTIFICATION, crtx_evt_notification);
// 	
// 	crtx_create_task(global_notify_graph, 0, "sd_bus_notify_send", &notify_send_handler, slistener);
// 	
// 	return &slistener->base;
// }

void crtx_sdbus_notifications_init() {
	#if 0
	// TODO unregister on shutdown
	crtx_register_handler_for_event_type("cortex.user_notification", "dbus_notification", &notify_send_handler, 0);
	#endif
}

void crtx_sdbus_notifications_finish() {
// 	if (def_bus) {
// 		sd_bus_flush(def_bus);
// 		sd_bus_unref(def_bus);
// // 		sd_bus_close(def_bus);
// 	}
}
