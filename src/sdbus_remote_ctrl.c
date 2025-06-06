/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "intern.h"
#include "core.h"
#include "sdbus.h"


// busctl --user tree org.cortexd.main
// busctl --user introspect org.cortexd.main /org/cortexd/main
// busctl --user call org.cortexd.main /org/cortexd/main org.cortexd.main crtx_add_event ss blubber data

// busctl --user call org.cortexd.main /org/cortexd/main org.cortexd.main add_signal s test
// busctl --user call org.cortexd.main /org/cortexd/main org.cortexd.main add_listener ss "/Mixers/PulseAudio__Playback_Devices_1" test
// dbus-monitor type=signal interface="org.cortexd.dbus.signal"

// 	
// 	slot = sd_bus_get_current_slot(bus);
// 	
// 	// 	r = sd_bus_add_match(bus, NULL, "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'", match_callback, NULL);
// 	// 	sd_bus_add_match(bus, &slot, "type='signal',path='/Mixers/PulseAudio__Playback_Devices_1'" , bus_signal_cb, bus);
// 	sd_bus_add_match(bus, &slot, "type='signal'" , bus_signal_cb, dthread);


sd_bus *sdbus_main_bus;
// extern sd_bus *sd_bus_main_bus;

static int sd_bus_crtx_add_event(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	char *type, *data;
	int r;
	struct crtx_event *event;
	struct sd_bus_userdata *args;
	
	args = (struct sd_bus_userdata*) userdata;
	
	/* Read the parameters */
	r = sd_bus_message_read(m, "ss", &type, &data);
	if (r < 0) {
		fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
		return r;
	}
	
	printf("received %s %s\n", type, data);
	
	event = crtx_create_event(type, data, strlen(data)+1);
	
	crtx_add_event(args->graph, event);
	
	return sd_bus_reply_method_return(m, "");
}

struct crtx_listener_base_data {
	char *path;
	char *event_type;
};

struct crtx_listener_base_data **listeners = 0;
unsigned int n_listeners = 0;

char * new_strcpy(char *input) {
	char *result = malloc(strlen(input)+1);
	strcpy(result, input);
	return result;
}

static int sd_bus_listener_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	struct crtx_listener_base_data *listener;
	struct crtx_event *event;
	
	listener = (struct crtx_listener_base_data*) userdata;
	
	event = crtx_create_event(listener->event_type, m, 0);
	event->data.flags |= CRTX_DIF_DONT_FREE_DATA;
	
	sd_bus_print_msg(m);
	
	reference_event_release(event);
	
	crtx_find_graph_add_event(event);
	
	crtx_wait_on_event(event);
	
// 	event->data.= 0;
	
	dereference_event_release(event);
	
	return 0;
}

void sd_bus_add_signal_listener(sd_bus *bus, char *path, char *event_type) {
		struct crtx_listener_base_data *listener;
		char *buf;
		int len;
		
		listener = (struct crtx_listener_base_data*) calloc(1, sizeof(struct crtx_listener_base_data));
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
	
	static int sd_bus_add_signal_listener_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
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
		
		sd_bus_add_signal_listener(args->bus, path, event_type);
		
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

char submit_signal(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct sd_bus_signal *args;
	
	args = (struct sd_bus_signal*) userdata;
	
	printf("submit signal\n");
	sd_bus_emit_signal(args->bus, args->path, "org.cortexd.dbus.signal", "new_event", "ss", event->type, "TODO event data");
	
	return 1;
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
	}
	sign->path = buf;
	
	char **event_types = (char**) malloc(sizeof(char*)*2);
	event_types[0] = event_type;
	event_types[1] = 0;
	new_eventgraph(&args->graph, 0, event_types);
	
	struct crtx_task *etask = crtx_new_task();
	etask->handle = &submit_signal;
	etask->userdata = sign;
	
	args->graph->tasks = etask;
	
	
	return sd_bus_reply_method_return(m, "");
}

static const sd_bus_vtable cortexd_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("crtx_add_event", "ss", "", sd_bus_crtx_add_event, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("add_signal_listener", "ss", "", sd_bus_add_signal_listener_cb, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("add_signal", "s", "", sd_bus_add_signal, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_VTABLE_END
};

void *sd_bus_crtx_main_thread(void *data) {
	sd_bus_slot *slot = NULL;
	sd_bus *bus = NULL;
	int r;
	struct sd_bus_userdata cb_args;
	
	bus = sd_bus_main_bus;
	
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
	
	return 0;
}

