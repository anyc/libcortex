#include <stdio.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

#include "core.h"


static char * local_event_types[] = { "test" };
static unsigned n_local_event_types = 1;


struct dbus_thread {
	sd_bus *bus;
	struct event_graph *graph;
};

void handle_event(struct event *event) {
	uint8_t type;
	const char *sig;
	char *val;
	
	sd_bus_message *m = (sd_bus_message *) event->data;
	
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

// static int bus_signal_cb(sd_bus *bus, int ret, sd_bus_message *m, void *user_data)
static int bus_signal_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	// 	uint8_t type;
	// 	const char *sig;
	// 	char *val;
	struct dbus_thread * dthread;
	struct event *event;
	
	dthread = (struct dbus_thread*) userdata;
	
	event = (struct event*) malloc(sizeof(struct event));
	event->type = local_event_types[0];
	event->data = m;
	
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
}

void sd_bus_init() {
	sd_bus_message *m = NULL;
	sd_bus *bus = NULL;
	sd_bus *ubus = NULL;
	const char *path;
	int r;
	unsigned int i;
	
	struct event_graph *event_graph;
	
	for (i=0; i<n_local_event_types; i++) {
		add_event_type(local_event_types[i]);
	}
	
	new_eventgraph(&event_graph, 0);
	
	struct event_task *etask = (struct event_task*) malloc(sizeof(struct event_task));
	etask->handle = &handle_event;
	
	event_graph->entry_task = etask;
	
	/* Connect to the system bus */
	r = sd_bus_open_system(&bus);
	if (r < 0) {
		fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
		return;
	}
	
	r = sd_bus_open_user(&ubus);
	if (r < 0) {
		fprintf(stderr, "Failed to connect to user bus: %s\n", strerror(-r));
		return;
	}
	
	pthread_t uthread, sthread;
	struct dbus_thread duthread, dsthread;
	
	duthread.bus = ubus;
	duthread.graph = event_graph;
	dsthread.bus = bus;
	dsthread.graph = event_graph;
	
	pthread_create(&sthread, NULL, tmain, &dsthread);
	pthread_create(&uthread, NULL, tmain, &duthread);
	
	pthread_join(sthread, NULL);
	pthread_join(uthread, NULL);
	
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

#ifndef STATIC_SD_BUS
void init() {
	sd_bus_init();
}
#endif