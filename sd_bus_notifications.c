

u_int32_t send_notification(char *icon, char *title, char *text, char **actions) {
	int r;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *reply = NULL;
	char **ait;
	sd_bus *bus;
	
	r = sd_bus_open_user(&bus);
	if (r < 0) {
		printf("Failed to connect to system bus: %s\n", strerror(-r));
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
	
	#define SDCHECK(r) { if (r < 0) {printf("err %d %s (%d %d) %d\n", r, strerror(r), EINVAL, EPERM,  __LINE__);} }
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
	sd_bus_message *m = (sd_bus_message *) event->data;
	
	sd_bus_print_msg(m);
}


void handle_cortexd_module_initialized(struct event *event, void *userdata, void *sessiondata) {
	printf("handle_cortexd_module_initialized\n");
}


#define EV_NOTIF_AC_INVOKED "sd-bus.org.freedesktop.Notifications.ActionInvoked"
char *notif_event_types[] = { EV_NOTIF_AC_INVOKED, 0 };

void send_query(char *icon, char *title, char *text, char **actions, char**chosen_action) {
	u_int32_t id;
	
	id = send_notification(icon, title, text, actions);
	
	struct event_graph *graph;
	
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
	
	
	
	// 	struct event_task *etask = new_task();
	// 	etask->handle = &handle_cortexd_enable;
	
}

