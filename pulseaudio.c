
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "pulseaudio.h"

char *crtx_pa_msg_etype[] = { "pulseaudio/event", 0 };

static struct crtx_sdbus_match matches[] = {
	{ "type='signal',sender='org.PulseAudio.Core1',interface='org.PulseAudio.Core1',path='/org/pulseaudio/core1',member='NewPlaybackStream'", "sd_bus/signal", 0 },
};

void crtx_free_pa_listener(struct crtx_listener_base *data) {
// 	struct crtx_pa_listener *palist;
	
// 	palist = (struct crtx_pa_listener*) data;
	
}

struct crtx_listener_base *crtx_new_pa_listener(void *options) {
	struct crtx_pa_listener *palist;
// 	pa_query_version_reply_t *version;
	int ret;
	struct crtx_listener_base *sdbus_lbase;
	struct crtx_sdbus_listener *def_sdbus_list;
	
	
	palist = (struct crtx_pa_listener *) options;
	
	
	def_sdbus_list = crtx_sdbus_get_default_listener(CRTX_SDBUS_TYPE_USER);
	if (!def_sdbus_list) {
		ERROR("crtx_sdbus_get_default_listener failed\n");
		return 0;
	}
	
	char *address;
	crtx_sdbus_get_property_str(def_sdbus_list,
							"org.PulseAudio1",
							 "/org/pulseaudio/server_lookup1",
							 "org.PulseAudio.ServerLookup1",
							 "Address", &address);
	
	memset(&palist->sdbus_list, 0, sizeof(struct crtx_sdbus_listener));
	palist->sdbus_list.bus_type = CRTX_SDBUS_TYPE_CUSTOM;
	palist->sdbus_list.name = address;
	
	palist->sdbus_list.matches = matches;
	
	sdbus_lbase = create_listener("sdbus", &palist->sdbus_list);
	if (!sdbus_lbase) {
		ERROR("create_listener(sdbus) failed\n");
		exit(1);
	}
	
	ret = crtx_start_listener(sdbus_lbase);
	if (!ret) {
		ERROR("starting sdbus listener failed\n");
		return 0;
	}
	
	palist->parent.free = &crtx_free_pa_listener;
	new_eventgraph(&palist->parent.graph, "pulseaudio", 0);
	
// 	palist->parent.thread = get_thread(pa_tmain, palist, 0);
	
	return &palist->parent;
}

void crtx_pa_init() {
}

void crtx_pa_finish() {
}


#ifdef CRTX_TEST

static char pa_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct crtx_dict *dict;
// 	struct crtx_pa_listener *palist;
	
	
// 	palist = (struct crtx_pa_listener *) event->origin;
	
	sd_bus_message *m = event->data.raw.pointer;
	sd_bus_print_msg(m);
// 	crtx_print_dict(dict);
	
// 	crtx_free_dict(dict);
	
	return 0;
}

int pa_main(int argc, char **argv) {
	struct crtx_pa_listener palist;
	struct crtx_listener_base *lbase;
	char ret;
	
	crtx_root->no_threads = 1;
	
	memset(&palist, 0, sizeof(struct crtx_pa_listener));
	
	
	
	// 	{ "type='signal',path='/org/pulseaudio/core1',interface='org.PulseAudio.Core1.NewPlaybackStream'", 0 },
	// 	{ "type='signal',path='/org/pulseaudio/core1'", 0 },
	
// 	r = sd_bus_call_method(bus,
// 						   "org.freedesktop.systemd1",           /* service to contact */
// 						"/org/freedesktop/systemd1",          /* object path */
// 						"org.freedesktop.systemd1.Manager",   /* interface name */
// 						"StartUnit",                          /* method name */
// 						&error,                               /* object to return error in */
// 						&m,                                   /* return message on success */
// 						"ss",                                 /* input signature */
// 						"cups.service",                       /* first argument */
// 						"replace");                           /* second argument */
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
// 	
// 	printf("Queued service job as %s.\n", path);
// 	
// 	dbus_addr = _sbus.get_object('org.PulseAudio1', '/org/pulseaudio/server_lookup1').Get('org.PulseAudio.ServerLookup1', 'Address', dbus_interface='org.freedesktop.DBus.Properties')
// 	
	
	
	lbase = create_listener("pulseaudio", &palist);
	if (!lbase) {
		ERROR("create_listener(pulseaudio) failed\n");
		exit(1);
	}
	
	crtx_create_task(lbase->graph, 0, "pa_test", pa_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting pulseaudio listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	free_listener(lbase);
	
	return 0;  
}

CRTX_TEST_MAIN(pa_main);

#endif
