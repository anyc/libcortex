#include <stdio.h>
#include <stdlib.h>

#include "intern.h"
#include "core.h"
#include "sdbus.h"
#include "threads.h"

// busctl --user tree org.cortexd.main
// busctl --user introspect org.cortexd.main /org/cortexd/main
// busctl --user call org.cortexd.main /org/cortexd/main org.cortexd.main crtx_add_event ss blubber data

// busctl --user call org.cortexd.main /org/cortexd/main org.cortexd.main add_signal s test
// busctl --user call org.cortexd.main /org/cortexd/main org.cortexd.main add_listener ss "/Mixers/PulseAudio__Playback_Devices_1" test
// dbus-monitor type=signal interface="org.cortexd.dbus.signal"



sd_bus *sd_bus_main_bus;

// struct sd_bus_userdata {
// 	sd_bus *bus;
// 	struct crtx_graph *graph;
// };



void sd_bus_print_msg(sd_bus_message *m) {
	const char *sig;
// 	char *val;
	
	sig = sd_bus_message_get_signature(m, 1);
	
	printf("sd_bus_message %s %s %s %s %s \"%s\"\n",
		  sd_bus_message_get_path(m),
		  sd_bus_message_get_interface(m),
		  sd_bus_message_get_member(m),
		  sd_bus_message_get_destination(m),
		  sd_bus_message_get_sender(m),
		  sig
	);
	
	if (sig[0] == 's') {
// 		sd_bus_message_read_basic(m, 's', &val);
// 		printf("\tval %s\n", val);
	}
}

// static int sd_bus_crtx_add_event(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
// 	char *type, *data;
// 	int r;
// 	struct crtx_event *event;
// 	struct sd_bus_userdata *args;
// 	
// 	args = (struct sd_bus_userdata*) userdata;
// 	
// 	/* Read the parameters */
// 	r = sd_bus_message_read(m, "ss", &type, &data);
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
// 		return r;
// 	}
// 	
// 	printf("received %s %s\n", type, data);
// 	
// 	event = crtx_create_event(type, data, strlen(data)+1);
// 	
// 	crtx_add_event(args->graph, event);
// 	
// 	return sd_bus_reply_method_return(m, "");
// }
// 
// struct crtx_listener_base_data {
// 	char *path;
// 	char *event_type;
// };
// 
// struct crtx_listener_base_data **listeners = 0;
// unsigned int n_listeners = 0;
// 
// char * new_strcpy(char *input) {
// 	char *result = malloc(strlen(input)+1);
// 	strcpy(result, input);
// 	return result;
// }
// 
// static int sd_bus_listener_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
// 	struct crtx_listener_base_data *listener;
// 	struct crtx_event *event;
// 	
// 	listener = (struct crtx_listener_base_data*) userdata;
// 	
// 	event = crtx_create_event(listener->event_type, m, 0);
// 	event->data.flags |= CRTX_DIF_DONT_FREE_DATA;
// 	
// 	sd_bus_print_msg(m);
// 	
// 	reference_event_release(event);
// 	
// 	add_raw_event(event);
// 	
// 	wait_on_event(event);
// 	
// // 	event->data.= 0;
// 	
// 	dereference_event_release(event);
// 	
// 	return 0;
// }
// 
void sd_bus_add_signal_listener(sd_bus *bus, char *path, char *event_type) {
// 	struct crtx_listener_base_data *listener;
// 	char *buf;
// 	int len;
// 	
// 	listener = (struct crtx_listener_base_data*) calloc(1, sizeof(struct crtx_listener_base_data));
// 	listener->path = new_strcpy(path);
// 	listener->event_type = new_strcpy(event_type);
// 	
// 	// 	TODO escape path
// 	len = strlen(path);
// 	buf = (char*) malloc(strlen("type='signal',path='")+len+2);
// 	sprintf(buf, "type='signal',path='%s'", path);
// 	
// 	printf("sd_bus will listen on %s\n", path);
// 	
// 	sd_bus_add_match(bus, NULL, buf, sd_bus_listener_cb, listener);
// 	
// 	free(buf);
// }
// 
// static int sd_bus_add_signal_listener_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
// 	char *path, *event_type;
// 	int r;
// 	struct sd_bus_userdata *args;
// 	
// 	args = (struct sd_bus_userdata*) userdata;
// 	
// 	/* Read the parameters */
// 	r = sd_bus_message_read(m, "ss", &path, &event_type);
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
// 		return r;
// 	}
// 	
// 	sd_bus_add_signal_listener(args->bus, path, event_type);
// 	
// 	return sd_bus_reply_method_return(m, "");
}
// 
// 
// 
// struct sd_bus_signal {
// 	sd_bus *bus;
// 	char *event_type;
// 	char *path;
// 	sd_bus_vtable *vtable;
// };
// 
// struct sd_bus_signal *signals = 0;
// unsigned int n_signals = 0;
// 
// char submit_signal(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct sd_bus_signal *args;
// 	
// 	args = (struct sd_bus_signal*) userdata;
// 	
// 	printf("submit signal\n");
// 	sd_bus_emit_signal(args->bus, args->path, "org.cortexd.dbus.signal", "new_event", "ss", event->type, "TODO event data");
// 	
// 	return 1;
// }
// 
// static int sd_bus_add_signal(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
// 	char *event_type, *buf;
// 	int r, len;
// 	struct sd_bus_userdata *args;
// 	
// 	args = (struct sd_bus_userdata*) userdata;
// 	
// 	/* Read the parameters */
// 	r = sd_bus_message_read(m, "s", &event_type);
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
// 		return r;
// 	}
// 	
// 	struct sd_bus_signal *sign;
// 	
// 	n_signals++;
// 	signals = (struct sd_bus_signal*) realloc(signals, sizeof(struct sd_bus_signal)*n_signals);
// 	
// 	sign = &signals[n_signals-1];
// 	
// 	sign->bus = args->bus;
// 	sign->event_type = new_strcpy(event_type);
// 	sign->vtable = (sd_bus_vtable*) malloc(sizeof(sd_bus_vtable)*3);
// 	sign->vtable[0] = (sd_bus_vtable) SD_BUS_VTABLE_START(0);
// 	sign->vtable[1] = (sd_bus_vtable) SD_BUS_SIGNAL("new_event", "ss", 0);
// 	sign->vtable[2] = (sd_bus_vtable) SD_BUS_VTABLE_END;
// 	
// 	#define SD_BUS_SIGNAL_PATH "/org/cortexd/events/"
// 	len = strlen(event_type);
// 	buf = (char*) malloc(strlen(SD_BUS_SIGNAL_PATH)+len+1);
// 	sprintf(buf, SD_BUS_SIGNAL_PATH "%s", event_type);
// 	
// 	/* Install the object */
// 	r = sd_bus_add_object_vtable(args->bus,
// 						NULL,
// 						buf,  /* object path */
// 						"org.cortexd.dbus.signal",   /* interface name */
// 						sign->vtable,
// 						args);
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to issue method call: %s\n", strerror(-r));
// 	}
// 	sign->path = buf;
// 	
// 	char **event_types = (char**) malloc(sizeof(char*)*2);
// 	event_types[0] = event_type;
// 	event_types[1] = 0;
// 	new_eventgraph(&args->graph, 0, event_types);
// 	
// 	struct crtx_task *etask = new_task();
// 	etask->handle = &submit_signal;
// 	etask->userdata = sign;
// 	
// 	args->graph->tasks = etask;
// 	
// 	
// 	return sd_bus_reply_method_return(m, "");
// }
// 
// static const sd_bus_vtable cortexd_vtable[] = {
// 	SD_BUS_VTABLE_START(0),
// 	SD_BUS_METHOD("crtx_add_event", "ss", "", sd_bus_crtx_add_event, SD_BUS_VTABLE_UNPRIVILEGED),
// 	SD_BUS_METHOD("add_signal_listener", "ss", "", sd_bus_add_signal_listener_cb, SD_BUS_VTABLE_UNPRIVILEGED),
// 	SD_BUS_METHOD("add_signal", "s", "", sd_bus_add_signal, SD_BUS_VTABLE_UNPRIVILEGED),
// 	SD_BUS_VTABLE_END
// };
// 
// void *sd_bus_crtx_main_thread(void *data) {
// 	sd_bus_slot *slot = NULL;
// 	sd_bus *bus = NULL;
// 	int r;
// 	struct sd_bus_userdata cb_args;
// 	
// 	bus = sd_bus_main_bus;
// 	
// 	cb_args.bus = bus;
// 	
// 	/* Install the object */
// 	r = sd_bus_add_object_vtable(bus,
// 						    &slot,
// 						"/org/cortexd/main",  /* object path */
// 						"org.cortexd.main",   /* interface name */
// 						cortexd_vtable,
// 						&cb_args);
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to issue method call: %s\n", strerror(-r));
// 		goto finish;
// 	}
// 	
// 	/* Take a well-known service name so that clients can find us */
// 	r = sd_bus_request_name(bus, "org.cortexd.main", 0);
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
// 		goto finish;
// 	}
// 	
// 	for (;;) {
// 		/* Process requests */
// 		r = sd_bus_process(bus, NULL);
// 		if (r < 0) {
// 			fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
// 			goto finish;
// 		}
// 		if (r > 0) /* we processed a request, try to process another one, right-away */
// 			continue;
// 		
// 		/* Wait for the next request to process */
// 		r = sd_bus_wait(bus, (uint64_t) -1);
// 		if (r < 0) {
// 			fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
// 			goto finish;
// 		}
// 	}
// 	
// finish:
// 	sd_bus_slot_unref(slot);
// 	sd_bus_unref(bus);
// 	
// 	return 0;
// }



// 	
// 	slot = sd_bus_get_current_slot(bus);
// 	
// 	// 	r = sd_bus_add_match(bus, NULL, "type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'", match_callback, NULL);
// 	// 	sd_bus_add_match(bus, &slot, "type='signal',path='/Mixers/PulseAudio__Playback_Devices_1'" , bus_signal_cb, bus);
// 	sd_bus_add_match(bus, &slot, "type='signal'" , bus_signal_cb, dthread);

int crtx_sd_bus_message_read_string(sd_bus_message *m, char **p) {
	char *o = 0;
	int r;
	
	r = sd_bus_message_read_basic(m, 's', &o);
	if (r >= 0) {
		if (o == 0)
			return -1;
		
		*p = crtx_stracpy(o, 0);
	}
	
	return r;
}

static void cb_event_release(struct crtx_event *event, void *userdata) {
	sd_bus_message_unref(event->data.pointer);
}

static int sdbus_match_listener_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	struct crtx_sdbus_match *match;
	struct crtx_event *event;
	
	
	match = (struct crtx_sdbus_match*) userdata;
	
	event = crtx_create_event(match->event_type);
// 	event->data.flags |= CRTX_DIF_DONT_FREE_DATA;
	crtx_event_set_raw_data(event, 'p', m, sizeof(m), CRTX_DIF_DONT_FREE_DATA);
	event->cb_before_release = &cb_event_release;
	
	sd_bus_print_msg(m);
	
	sd_bus_message_ref(m);
	
	crtx_add_event(match->listener->parent.graph, event);
	
	return 0;
}

// #include <poll.h>
// int crtx_sd_bus_get_events(sd_bus *bus) {
// 	int f, result;
// 	
// 	result = 0;
// 	
// 	f = sd_bus_get_events(bus);
// 	if (f & POLLIN)
// 		result |= EPOLLIN;
// 	if (f & POLLOUT)
// 		result |= EPOLLOUT;
// 	
// 	return result;
// }

static char sdbus_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *payload;
	struct crtx_sdbus_listener *sdlist;
	int r;
	
	payload = (struct crtx_event_loop_payload*) event->data.pointer;
	
	sdlist = (struct crtx_sdbus_listener *) payload->data;
	
	while (1) {
		r = sd_bus_process(sdlist->bus, NULL);
		if (r < 0) {
			ERROR("sd_bus_process failed: %s\n", strerror(-r));
			break;
		}
		if (r == 0)
			break;
	}
	
// 	int new_flags;
// 	new_flags = crtx_sd_bus_get_events(sdlist->bus);
// 	if (payload->event_flags != new_flags) {
// 		printf("%d != %d\n", payload->event_flags, new_flags);
// 		payload->event_flags = new_flags;
// 	}
	
	return r;
}

void crtx_shutdown_sdbus_listener(struct crtx_listener_base *data) {
	struct crtx_sdbus_listener *sdlist;
	
	sdlist = (struct crtx_sdbus_listener*) data;
	
	sd_bus_unref(sdlist->bus);
}

char crtx_open_sdbus(sd_bus **bus, enum crtx_sdbus_type bus_type, char *name) {
	int r;
	
	switch (bus_type) {
		case CRTX_SDBUS_TYPE_DEFAULT:
			r = sd_bus_open(bus);
			break;
		case CRTX_SDBUS_TYPE_USER:
			r = sd_bus_open_user(bus);
			break;
		case CRTX_SDBUS_TYPE_SYSTEM:
			r = sd_bus_open_system(bus);
			break;
		case CRTX_SDBUS_TYPE_SYSTEM_REMOTE:
			r = sd_bus_open_system_remote(bus, name);
			break;
		case CRTX_SDBUS_TYPE_SYSTEM_MACHINE:
			r = sd_bus_open_system_machine(bus, name);
			break;
		case CRTX_SDBUS_TYPE_CUSTOM:
			r = sd_bus_new(bus);
			if (r >= 0) {
				int t;
				
				DBG("using custom DBUS at address \"%s\"\n", name);
				
				t = sd_bus_set_address(*bus, name);
				if (t < 0) {
					fprintf(stderr, "sd_bus_set_address failed: %s\n", strerror(-t));
					return t;
				}
				
				t = sd_bus_start(*bus);
				if (t < 0) {
					fprintf(stderr, "sd_bus_start failed: %s\n", strerror(-t));
					return t;
				}
			}
			break;
		default:
			ERROR("crtx_new_sdbus_listener\n");
			return -1;
	}
	if (r < 0) {
		fprintf(stderr, "Failed to open bus: %s\n", strerror(-r));
		return r;
	}
	
	return 0;
}

struct crtx_listener_base *crtx_new_sdbus_listener(void *options) {
	struct crtx_sdbus_listener *sdlist;
	struct crtx_sdbus_match *m;
	int r;
	
	sdlist = (struct crtx_sdbus_listener *) options;
	
	if (!sdlist->bus) {
		if (crtx_open_sdbus(&sdlist->bus, sdlist->bus_type, sdlist->name) < 0)
			return 0;
	}
	
// 	int flags = sd_bus_get_events(sdlist->bus);
// 	int eflags = 0;
// 	if (flags | POLLIN)
// 		eflags |= EPOLLIN;
// 	if (flags | POLLOUT)
// 		eflags |= EPOLLOUT;
	
	sdlist->parent.el_payload.fd = sd_bus_get_fd(sdlist->bus);
// 	sdlist->parent.el_payload.event_flags = EPOLLIN | crtx_sd_bus_get_events(sdlist->bus);
// 	sdlist->parent.el_payload.event_flags = EPOLLIN | EPOLLOUT;
	sdlist->parent.el_payload.data = sdlist;
	sdlist->parent.el_payload.event_handler = &sdbus_fd_event_handler;
	sdlist->parent.el_payload.event_handler_name = "sdbus event handler";
	
	sdlist->parent.shutdown = &crtx_shutdown_sdbus_listener;
// 	new_eventgraph(&sdlist->parent.graph, 0, 0);
	
	m = sdlist->matches;
	while (m && m->match_str) {
		printf("sd_bus will listen for \"%s\" -> %s\n", m->match_str, m->event_type);
		m->listener = sdlist;
		
		r = sd_bus_add_match(sdlist->bus, NULL, m->match_str, sdbus_match_listener_cb, m);
		if (r < 0) {
			fprintf(stderr, "sd_bus_add_match failed: %s\n", strerror(-r));
			return 0;
		}
		
		m++;
	}
	
	return &sdlist->parent;
}

// const char *destination, const char *path, const char *interface, const char *member, sd_bus_error *ret_error, sd_bus_message **reply, const char *types, ...
void crtx_sdbus_call_method(struct crtx_sdbus_listener *sdlist, 
				char *destination,
				char *path,
				char *interface,
				char *member,
				struct crtx_dict *dict
				)
{
// 	int r;
// 	sd_bus_message *m;
	
// 	('org.PulseAudio1', '/org/pulseaudio/server_lookup1').Get('org.PulseAudio.ServerLookup1', 'Address', dbus_interface='org.freedesktop.DBus.Properties')
	
// 	r = sd_bus_message_new_method_call(sdlist->bus,
// 									   &m,
// 									"org.PulseAudio1",  /* service to contact */
// 									"/org/pulseaudio/server_lookup1", /* object path */
// 									"org.PulseAudio.ServerLookup1",  /* interface name */
// 									"Address"                          /* method name */
// 	);
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to issue method call: %s\n", error.message);
// 		goto finish;
// 	}
// 	
// 	/* Parse the response message */
// 	r = sd_bus_message_read(m, "o", &path);
// 	if (r < 0) {
// 		fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
// 		goto finish;
// 	}
}

int crtx_sdbus_get_property_str(struct crtx_sdbus_listener *sdlist, 
							char *destination,
							char *path,
							char *interface,
							char *member,
							char **s
					)
{
	int r;
	sd_bus_error ret_error;
	
	memset(&ret_error, 0, sizeof(sd_bus_error));
	
// 	sd_bus *bus;
// 	r = sd_bus_open_user(&bus);
// 	if (r< 0)
// 		exit(1);
	r = sd_bus_get_property_string(sdlist->bus, destination, path, interface, member, &ret_error, s);
// 	r = sd_bus_get_property_string(bus, "org.pulseaudio.Server1",
// 								   "/org/pulseaudio/server_lookup1",
// 								"org.PulseAudio.ServerLookup1",
// 								"Address", &ret_error, s);
	if (r < 0) {
		fprintf(stderr, "sd_bus_get_property_string failed: %s\n", strerror(-r));
	}
	
	return r;
}

static struct crtx_sdbus_listener default_listeners[CRTX_SDBUS_TYPE_MAX] = { { { 0 } } };


struct crtx_sdbus_listener *crtx_sdbus_get_default_listener(enum crtx_sdbus_type sdbus_type) {
	if (!default_listeners[sdbus_type].bus) {
		struct crtx_listener_base *lbase;
		int ret;
		
		
		default_listeners[sdbus_type].bus_type = sdbus_type;
		
		lbase = create_listener("sdbus", &default_listeners[sdbus_type]);
		if (!lbase) {
			return 0;
		}
		
		ret = crtx_start_listener(lbase);
		if (!ret) {
			return 0;
		}
	}
	
	return &default_listeners[sdbus_type];
}

void crtx_sdbus_init() {
// 	int r;
	
// 	r = sd_bus_open_user(&sd_bus_main_bus);
// 	if (r < 0) {
// 		printf("sd_bus: failed to open bus: %s\n", strerror(-r));
// 		return;
// 	}
}

void crtx_sdbus_finish() {
	int i;
	
	for (i=0; i < CRTX_SDBUS_TYPE_MAX; i++) {
		if (default_listeners[i].bus)
			crtx_free_listener(&default_listeners[i].parent);
	}
	
// 	sd_bus_flush(sd_bus_main_bus);
// 	sd_bus_unref(sd_bus_main_bus);
}

#ifndef STATIC_SD_BUS
void init() {
// 	sd_bus_init();
}
#endif


#ifdef CRTX_TEST
static char sdbus_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	// 	struct sdbus_device *dev, *usb_dev;
// 	struct crtx_dict *dict;
	
	// 	dev = (struct sdbus_device *) event->data.pointer;
	
// 	dict = sdbus_raw2dict(event, r2ds);
	
// 	crtx_print_dict(dict);
	
// 	crtx_free_dict(dict);
	
	return 0;
}

struct crtx_sdbus_match matches[] = {
	{ "type='signal'", "sd_bus/signal", 0 },
	{0},
};

int sdbus_main(int argc, char **argv) {
	struct crtx_sdbus_listener sdlist;
	struct crtx_listener_base *lbase;
	char ret;
	
	
// 	crtx_root->no_threads = 1;
	
	memset(&sdlist, 0, sizeof(struct crtx_sdbus_listener));
	
	sdlist.bus_type = CRTX_SDBUS_TYPE_USER;
	sdlist.matches = matches;
	
	lbase = create_listener("sdbus", &sdlist);
	if (!lbase) {
		ERROR("create_listener(sdbus) failed\n");
		exit(1);
	}
	
	crtx_create_task(lbase->graph, 0, "sdbus_test", sdbus_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting sdbus listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	return 0;  
}

CRTX_TEST_MAIN(sdbus_main);

#endif
