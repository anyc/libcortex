#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "sd_bus.h"

// busctl --user tree org.cortexd.main
// busctl --user introspect org.cortexd.main /org/cortexd/main
// busctl --user call org.cortexd.main /org/cortexd/main org.cortexd.main add_event ss blubber data

// busctl --user call org.cortexd.main /org/cortexd/main org.cortexd.main add_signal s test
// busctl --user call org.cortexd.main /org/cortexd/main org.cortexd.main add_listener ss "/Mixers/PulseAudio__Playback_Devices_1" test
// dbus-monitor type=signal interface="org.cortexd.dbus.signal"

static char * local_event_types[] = { "test", 0 };

static pthread_t dthread;
struct dbus_thread duthread, dsthread, ddthread;

sd_bus *sd_bus_main_bus;

struct dbus_thread {
	sd_bus *bus;
	struct event_graph *graph;
};





struct sd_bus_userdata {
	sd_bus *bus;
	struct event_graph *graph;
};

void sd_bus_print_msg(sd_bus_message *m) {
	const char *sig;
	char *val;
	
	sig = sd_bus_message_get_signature(m, 1);
	
	printf("got bus signal... %s %s %s %s %s \"%s\"\n",
		  sd_bus_message_get_path(m),
		  sd_bus_message_get_interface(m),
		  sd_bus_message_get_member(m),
		  sd_bus_message_get_destination(m),
		  sd_bus_message_get_sender(m),
		  sig
	);
	
	if (sig[0] == 's') {
		sd_bus_message_read_basic(m, 's', &val);
		printf("\tval %s\n", val);
	}
}

static int sd_bus_add_event(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	char *type, *data;
	int r;
	struct event *event;
	struct sd_bus_userdata *args;
	
	args = (struct sd_bus_userdata*) userdata;
	
	/* Read the parameters */
	r = sd_bus_message_read(m, "ss", &type, &data);
	if (r < 0) {
		fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
		return r;
	}
	
	printf("received %s %s\n", type, data);
	
// 	event = new_event();
// 	event->type = type;
// 	event->data = data;
	event = create_event(type, data, strlen(data)+1);
	
	add_event(args->graph, event);
	
	return sd_bus_reply_method_return(m, "");
}

struct listener_data {
	char *path;
	char *event_type;
};

struct listener_data **listeners = 0;
unsigned int n_listeners = 0;

char * new_strcpy(char *input) {
	char *result = malloc(strlen(input)+1);
	strcpy(result, input);
	return result;
}

static int sd_bus_listener_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	struct listener_data *listener;
	struct event *event;
	
	listener = (struct listener_data*) userdata;
	
// 	event = new_event();
// 	event->type = listener->event_type;
// 	event->data = m;
	event = create_event(listener->event_type, m, 0);
	
	sd_bus_print_msg(m);
	
	// 	sig = sd_bus_message_get_signature(m, 1);
	// 	
	// 	printf("got bus signal... %s %s %s %s %s \"%s\"\n",
	// 		  sd_bus_message_get_path(m),
	// 		  sd_bus_message_get_interface(m),
	// 		  sd_bus_message_get_member(m),
	// 		  sd_bus_message_get_destination(m),
	// 		  sd_bus_message_get_sender(m),
	// 		  sig
	// 	);
	// 	
	// 	if (sig[0] == 's') {
	// 		sd_bus_message_read_basic(m, 's', &val);
	// 		printf("\tval %s\n", val);
	// 	}
	
	add_raw_event(event);
	
	wait_on_event(event);
	
	return 0;
}

void sd_bus_add_listener(sd_bus *bus, char *path, char *event_type) {
	struct listener_data *listener;
	char *buf;
	int len;
	
	listener = (struct listener_data*) calloc(1, sizeof(struct listener_data));
	listener->path = new_strcpy(path);
	listener->event_type = new_strcpy(event_type);
	
	// 	TODO escape path
	len = strlen(path);
	buf = (char*) malloc(strlen("type='signal',path='")+len+2);
	sprintf(buf, "type='signal',path='%s'", path);
	
	printf("sd_bus will listen on %s\n", path);
	
	sd_bus_add_match(bus, NULL, buf, sd_bus_listener_cb, listener);
	
	free(buf);
}

static int sd_bus_add_listener_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	char *path, *event_type;
	int r;
	struct sd_bus_userdata *args;
	
	args = (struct sd_bus_userdata*) userdata;
	
	/* Read the parameters */
	r = sd_bus_message_read(m, "ss", &path, &event_type);
	if (r < 0) {
		fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
		return r;
	}
	
	sd_bus_add_listener(args->bus, path, event_type);
	
	return sd_bus_reply_method_return(m, "");
}



struct sd_bus_signal {
	sd_bus *bus;
	char *event_type;
	char *path;
	sd_bus_vtable *vtable;
};

struct sd_bus_signal *signals = 0;
unsigned int n_signals = 0;

void submit_signal(struct event *event, void *userdata, void **sessiondata) {
// 	sd_bus_message *m = (sd_bus_message *) event->data;
	
// 	print_sd_bus_msg(m);
	struct sd_bus_signal *args;
	
	args = (struct sd_bus_signal*) userdata;
	
	printf("submit signal\n");
	sd_bus_emit_signal(args->bus, args->path, "org.cortexd.dbus.signal", "new_event", "ss", event->type, "TODO event data");
}

static int sd_bus_add_signal(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	char *event_type, *buf;
	int r, len;
	struct sd_bus_userdata *args;
	
	args = (struct sd_bus_userdata*) userdata;
	
	/* Read the parameters */
	r = sd_bus_message_read(m, "s", &event_type);
	if (r < 0) {
		fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
		return r;
	}
	
	struct sd_bus_signal *sign;
	
	n_signals++;
	signals = (struct sd_bus_signal*) realloc(signals, sizeof(struct sd_bus_signal)*n_signals);
// 	sign = (struct sd_bus_signal*) malloc(sizeof(struct sd_bus_signal));
	
	sign = &signals[n_signals-1];
	
	sign->bus = args->bus;
	sign->event_type = new_strcpy(event_type);
	sign->vtable = (sd_bus_vtable*) malloc(sizeof(sd_bus_vtable)*3);
	sign->vtable[0] = (sd_bus_vtable) SD_BUS_VTABLE_START(0);
	sign->vtable[1] = (sd_bus_vtable) SD_BUS_SIGNAL("new_event", "ss", 0);
	sign->vtable[2] = (sd_bus_vtable) SD_BUS_VTABLE_END;
	
	#define SD_BUS_SIGNAL_PATH "/org/cortexd/events/"
	len = strlen(event_type);
	buf = (char*) malloc(strlen(SD_BUS_SIGNAL_PATH)+len+1);
	sprintf(buf, SD_BUS_SIGNAL_PATH "%s", event_type);
	
	/* Install the object */
	r = sd_bus_add_object_vtable(args->bus,
						NULL,
						buf,  /* object path */
						"org.cortexd.dbus.signal",   /* interface name */
						sign->vtable,
						args);
	if (r < 0) {
		fprintf(stderr, "Failed to issue method call: %s\n", strerror(-r));
// 		goto finish;
	}
// 	free(buf);
	sign->path = buf;
	
	char **event_types = (char**) malloc(sizeof(char*)*2);
	event_types[0] = event_type;
	event_types[1] = 0;
	new_eventgraph(&args->graph, event_types);
	
	struct event_task *etask = new_task();
	etask->handle = &submit_signal;
	etask->userdata = sign;
	
	args->graph->tasks = etask;
	
	
	
	/* Take a well-known service name so that clients can find us */
// 	r = sd_bus_request_name(bus, "org.cortexd.dbus", 0);
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
// 		goto finish;
// 	}
	
	return sd_bus_reply_method_return(m, "");
}

static const sd_bus_vtable cortexd_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("add_event", "ss", "", sd_bus_add_event, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("add_listener", "ss", "", sd_bus_add_listener_cb, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("add_signal", "s", "", sd_bus_add_signal, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_VTABLE_END
};

void *sd_bus_daemon_thread(void *data) {
	sd_bus_slot *slot = NULL;
	sd_bus *bus = NULL;
	int r;
	struct dbus_thread *tdata;
	struct sd_bus_userdata cb_args;
	
	tdata = (struct dbus_thread*) data;
	
	r = sd_bus_open_user(&bus);
// 	ASSERT_STR(r >=0, "Failed to connect to system bus: %s\n", strerror(-r));
	if (r < 0) {
		printf("sd_bus: failed to open bus: %s\n", strerror(-r));
		return 0;
	}
	
	sd_bus_main_bus = bus;
	
	cb_args.graph = tdata->graph;
	cb_args.bus = bus;
	
	/* Install the object */
	r = sd_bus_add_object_vtable(bus,
						    &slot,
						"/org/cortexd/main",  /* object path */
						"org.cortexd.main",   /* interface name */
						cortexd_vtable,
						&cb_args);
	if (r < 0) {
		fprintf(stderr, "Failed to issue method call: %s\n", strerror(-r));
		goto finish;
	}
	
	/* Take a well-known service name so that clients can find us */
	r = sd_bus_request_name(bus, "org.cortexd.main", 0);
	if (r < 0) {
		fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
		goto finish;
	}
	
	for (;;) {
		/* Process requests */
		r = sd_bus_process(bus, NULL);
		if (r < 0) {
			fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
			goto finish;
		}
		if (r > 0) /* we processed a request, try to process another one, right-away */
			continue;
		
		/* Wait for the next request to process */
		r = sd_bus_wait(bus, (uint64_t) -1);
		if (r < 0) {
			fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
			goto finish;
		}
	}
	
finish:
	sd_bus_slot_unref(slot);
	sd_bus_unref(bus);
	
	// 	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
	return 0;
}




void sd_bus_print_event_task(struct event *event, void *userdata) {
	sd_bus_message *m = (sd_bus_message *) event->data.raw;
	
	sd_bus_print_msg(m);
}

// static int bus_signal_cb(sd_bus *bus, int ret, sd_bus_message *m, void *user_data)
static int bus_signal_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	// 	uint8_t type;
	// 	const char *sig;
	// 	char *val;
	struct dbus_thread * dthread;
	struct event *event;
	
	dthread = (struct dbus_thread*) userdata;
	
// 	event = new_event();
// 	event->type = local_event_types[0];
// 	event->data = m;
	event = create_event(local_event_types[0], m, 0);

	
	// 	sig = sd_bus_message_get_signature(m, 1);
	// 	
	// 	printf("got bus signal... %s %s %s %s %s \"%s\"\n",
	// 		  sd_bus_message_get_path(m),
	// 		  sd_bus_message_get_interface(m),
	// 		  sd_bus_message_get_member(m),
	// 		  sd_bus_message_get_destination(m),
	// 		  sd_bus_message_get_sender(m),
	// 		  sig
	// 	);
	// 	
	// 	if (sig[0] == 's') {
	// 		sd_bus_message_read_basic(m, 's', &val);
	// 		printf("\tval %s\n", val);
	// 	}
	
	add_event(dthread->graph, event);
	
	return 0;
}

void * tmain(void *data) {
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_slot* slot;
	sd_bus *bus;
	int r;
	struct dbus_thread * dthread;
	
	dthread = (struct dbus_thread*) data;
	bus = dthread->bus;
	
	slot = sd_bus_get_current_slot(bus);
	
	// 	r = sd_bus_add_match(bus, NULL, "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'", match_callback, NULL);
	// 	sd_bus_add_match(bus, &slot, "type='signal',path='/Mixers/PulseAudio__Playback_Devices_1'" , bus_signal_cb, bus);
	sd_bus_add_match(bus, &slot, "type='signal'" , bus_signal_cb, dthread);
	
	printf("listening...\n");
	while (1) {
		r = sd_bus_process(bus, NULL);
		if (r < 0) {
			fprintf(stderr, "sd_bus_process failed %d\n", r);
			break;
		}
		if (r == 0) {
			r = sd_bus_wait(bus, (uint64_t) -1);
			if (r < 0) {
				fprintf(stderr, "Failed to wait: %d", r);
				break;
			}
		}
	}
	
	sd_bus_flush(bus);
	sd_bus_error_free(&error);
	sd_bus_unref(bus);
	
	return 0;
}

void sd_bus_init() {
// 	sd_bus *bus = NULL;
	
// 	struct event_graph *event_graph;
// 	
// 	i=0;
// 	while (local_event_types[i]) {
// 		add_event_type(local_event_types[i]);
// 		i++;
// 	}
// 	
// 	new_eventgraph(&event_graph, local_event_types);
// 	
// 	struct event_task *etask = (struct event_task*) malloc(sizeof(struct event_task));
// 	etask->handle = &handle_event;
// 	
// 	event_graph->entry_task = etask;
// 	
// 	/* Connect to the system bus */
// 	r = sd_bus_open_system(&bus);
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
// 		return;
// 	}
// 	
// 	r = sd_bus_open_user(&ubus);
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to connect to user bus: %s\n", strerror(-r));
// 		return;
// 	}
// 	
// 	
// 	duthread.bus = ubus;
// 	duthread.graph = event_graph;
// 	dsthread.bus = bus;
// 	dsthread.graph = event_graph;
// 	
// 	pthread_create(&sthread, NULL, tmain, &dsthread);
// 	pthread_create(&uthread, NULL, tmain, &duthread);
	
	
	
	
// 	main_bus = bus;
// 	ddthread.bus = bus;
	ddthread.graph = cortexd_graph;
	
	pthread_create(&dthread, NULL, sd_bus_daemon_thread, &ddthread);
	
	
	
	// 	/* Issue the method call and store the respons message in m */
	// 	r = sd_bus_call_method(bus,
	// 						"org.freedesktop.UDisks2",           /* service to contact */
	// 						"/org/freedesktop/UDisks2/drives/hp________DDVDW_TS_H653R_R3786GASC5277600",          /* object path */
	// 						"org.freedesktop.DBus.Peer",   /* interface name */
	// 						"GetMachineId",                          /* method name */
	// 						&error,                               /* object to return error in */
	// 						&m,                                   /* return message on success */
	// 						"",                                 /* input signature */
	// 						"",                       /* first argument */
	// 						"");                           /* second argument */
	// 	if (r < 0) {
	// 			fprintf(stderr, "Failed to issue method call: %s\n", error.message);
	// 			goto finish;
	// 	}
	// 
	// 	/* Parse the response message */
	// 	r = sd_bus_message_read(m, "s", &path);
	// 	if (r < 0) {
	// 			fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
	// 			goto finish;
	// 	}
	// 
	// 	printf("Queued service job as %s.\n", path);
	
	// 	while (1) {
	// 		r = sd_bus_process(bus, NULL);
	// 		if (r < 0) {
	// 			fprintf(stderr, "sd_bus_process failed %d\n", r);
	// 			break;
	// 		}
	// 		if (r == 0) {
	// 			r = sd_bus_wait(bus, (uint64_t) -1);
	// 			if (r < 0) {
	// 				fprintf(stderr, "Failed to wait: %d", r);
	// 				goto finish;
	// 			}
	// 		}
	// 	}
	
	// finish:
	// 	sd_bus_flush(bus);
	// 	sd_bus_error_free(&error);
	// 	sd_bus_message_unref(m);
	// 	sd_bus_unref(bus);
	// 
	// 	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

void sd_bus_finish() {
// 	pthread_join(sthread, NULL);
// 	pthread_join(uthread, NULL);
	
	pthread_join(dthread, NULL);
}

#ifndef STATIC_SD_BUS
void init() {
	sd_bus_init();
}
#endif

// void sd_bus_call_method(sd_bus *bus, char *dest, char *path, char *interface, char *member, char *sign) {
// 	int r;
// 	sd_bus_error error = SD_BUS_ERROR_NULL;
// 	sd_bus_message *m = NULL;
// 	
// 	r = sd_bus_call_method(bus,
// 				"org.freedesktop.systemd1",           /* service to contact */
// 				"/org/freedesktop/systemd1",          /* object path */
// 				"org.freedesktop.systemd1.Manager",   /* interface name */
// 				"StartUnit",                          /* method name */
// 				&error,                               /* object to return error in */
// 				&m,                                   /* return message on success */
// 				"ss",                                 /* input signature */
// 				"cups.service",                       /* first argument */
// 				"replace");                           /* second argument */
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to issue method call: %s\n", error.message);
// 		goto finish;
// 	}
// 	
// 	
// 	r = sd_bus_message_new_method_call(bus, &m, dest, path, interface, member);
// }