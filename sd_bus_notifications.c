
#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "sd_bus.h"
#include "sd_bus_notifications.h"

#define EV_NOTIF_AC_INVOKED "sd-bus.org.freedesktop.Notifications.ActionInvoked"
char *notif_event_types[] = { EV_NOTIF_AC_INVOKED, 0 };

static char initialized = 0;
static struct event_graph *graph;

static u_int32_t *notif_ids = 0;
static size_t n_notif_ids = 0;
static struct signal *signals = 0;
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

void handle_notification_response(struct event *event, void *userdata, void **sessiondata) {
// 	sd_bus_message *m = (sd_bus_message *) event->data;
// 	size_t i;
// 	
// 	sd_bus_print_msg(m);
// 	
// 	pthread_mutex_lock(&notif_mutex);
// 	for (i=0; i<n_notif_ids; i++) {
// // 		if (notif_ids[i] == 
// 	}
// 	pthread_mutex_unlock(&notif_mutex);
}

void send_notification(char *icon, char *title, char *text, char **actions, char**chosen_action) {
	
	if (!initialized) {
		int ret;
		printf("initialize\n");
		ret = pthread_mutex_init(&notif_mutex, 0); ASSERT(ret >= 0);
		
		graph = get_graph_for_event_type(EV_NOTIF_AC_INVOKED);
		if (!graph) {
			new_eventgraph(&graph, notif_event_types);
		}
		
		sd_bus_add_listener(sd_bus_main_bus, "/org/freedesktop/Notifications", EV_NOTIF_AC_INVOKED);
		
		struct event_task *task2;
		task2 = new_task();
		task2->id = "notif_handler";
		task2->handle = &handle_notification_response;
		task2->userdata = 0;
		add_task(graph, task2);
		
		initialized = 1;
	}
	
	if (actions) {
		pthread_mutex_lock(&notif_mutex);
		
		n_notif_ids++;
		notif_ids = (u_int32_t*) realloc(signals, sizeof(u_int32_t)*n_notif_ids);
		signals = (struct signal*) realloc(signals, sizeof(struct signal)*n_notif_ids);
		init_signal(&signals[n_notif_ids-1]);
		pthread_mutex_unlock(&notif_mutex);
		
		notif_ids[n_notif_ids-1] = sd_bus_send_notification(icon, title, text, actions);
		
		wait_on_signal(&signals[n_notif_ids-1]);
	} else {
		sd_bus_send_notification(icon, title, text, actions);
	}
	
	// 	struct event_task *etask = new_task();
	// 	etask->handle = &handle_cortexd_enable;
	
}

static void notify_task_handler(struct event *event, void *userdata, void **sessiondata) {
// 	struct sd_bus_notifications_listener *slistener;
	
// 	slistener = (struct sd_bus_notifications_listener *) userdata;
	
	send_notification("", "test", (char*) event->data, 0, 0);
}

struct listener *new_sd_bus_notification_listener(void *options) {
	struct sd_bus_notifications_listener *slistener;
	
	slistener = (struct sd_bus_notifications_listener *) options;
	
	slistener->parent.graph = get_graph_for_event_type(EV_NOTIF_AC_INVOKED);
	if (!slistener->parent.graph) {
		new_eventgraph(&slistener->parent.graph, notif_event_types);
	}
	
	struct event_task * notify_task;
	notify_task = new_task();
	notify_task->id = "notify_task";
	notify_task->handle = &notify_task_handler;
	notify_task->userdata = slistener;
	add_task(slistener->parent.graph, notify_task);
	
	return &slistener->parent;
}
