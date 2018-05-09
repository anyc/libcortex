/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include "intern.h"
#include "core.h"
#include "sdbus.h"
#include "threads.h"

void crtx_sdbus_print_msg(sd_bus_message *m) {
	const char *sig;
	char *val;
	
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
		sd_bus_message_read_basic(m, 's', &val);
		printf("\tval %s\n", val);
	}
}

static void match_event_release(struct crtx_event *event, void *userdata) {
	sd_bus_message_unref(event->data.pointer);
}

static int match_event_cb_generic(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	struct crtx_sdbus_match *match;
	struct crtx_event *event;
	
	
	match = (struct crtx_sdbus_match*) userdata;
	
	event = crtx_create_event(match->event_type);
	
	crtx_event_set_raw_data(event, 'p', m, sizeof(m), CRTX_DIF_DONT_FREE_DATA);
	event->cb_before_release = &match_event_release;
	
	sd_bus_message_ref(m);
	
	crtx_add_event(match->listener->parent.graph, event);
	
	return 0;
}

static int match_event_cb_track(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	struct crtx_sdbus_track *track;
	char *peer, *old, *new;
	int r;
	
	
	track = (struct crtx_sdbus_track *) userdata;
	
	r = sd_bus_message_read_basic(m, 's', &peer);
	if (r < 0) {
		ERROR("sd_bus_message_read_basic failed: %s\n", strerror(r));
		return r;
	}
	r = sd_bus_message_read_basic(m, 's', &old);
	if (r < 0) {
		ERROR("sd_bus_message_read_basic failed: %s\n", strerror(r));
		return r;
	}
	r = sd_bus_message_read_basic(m, 's', &new);
	if (r < 0) {
		ERROR("sd_bus_message_read_basic failed: %s\n", strerror(r));
		return r;
	}
	
	if (strcmp(peer, track->peer)) {
		ERROR("received unexpected peer: \"%s\" != \"%s\"\n", peer, track->peer);
		return -1;
	}
	
	if (strlen(new) > 0 && strlen(old) == 0) {
		track->appear_cb(track, peer, new, track->userdata);
	}
	
	if (strlen(old) > 0 && strlen(new) == 0) {
		track->disappear_cb(track, peer, old, track->userdata);
	}
	
	return 0;
}

int crtx_sdbus_match_add(struct crtx_sdbus_listener *lstnr, struct crtx_sdbus_match *match) {
	int r;
	struct crtx_dll *dll;
	
	match->listener = lstnr;
	if (!match->callback) {
		match->callback = match_event_cb_generic;
		match->callback_data = match;
	}
	
	dll = crtx_dll_append_new(&lstnr->matches, match);
	
	DBG("sdbus will listen for \"%s\" (etype: \"%s\")\n", match->match_str, match->event_type);
	
	r = sd_bus_add_match(lstnr->bus, &match->slot, match->match_str, match->callback, match->callback_data);
	if (r < 0) {
		ERROR("sd_bus_add_match failed: %s\n", strerror(-r));
		
		crtx_dll_unlink(&lstnr->matches, dll);
		free(dll);
		
		return r;
	}
	
	return 0;
}

int crtx_sdbus_match_remove(struct crtx_sdbus_listener *lstnr, struct crtx_sdbus_match *match) {
	struct crtx_dll *dll;
	
	for (dll=lstnr->matches; dll && dll->data != match; dll=dll->next) {}
	if (dll) {
		crtx_dll_unlink(&lstnr->matches, dll);
		
		sd_bus_slot_unref(match->slot);
	} else {
		ERROR("did not find match obj in linked list\n");
	}
	
	return 0;
}

int crtx_sdbus_track_add(struct crtx_sdbus_listener *lstnr, struct crtx_sdbus_track *track) {
	int r;
	size_t match_len;
	
	
	#define TRACK_TEMPLATE "type='signal'," \
		"sender='org.freedesktop.DBus'," \
		"path='/org/freedesktop/DBus'," \
		"interface='org.freedesktop.DBus'," \
		"member='NameOwnerChanged'," \
		"arg0='%s'"
	
	match_len = strlen(TRACK_TEMPLATE) + strlen(track->peer);
	track->match.match_str = (char*) malloc(match_len);
	
	snprintf(track->match.match_str, match_len, TRACK_TEMPLATE, track->peer);
	
	track->match.callback = match_event_cb_track;
	track->match.callback_data = track;
	
	r = crtx_sdbus_match_add(lstnr, &track->match);
	if (r) {
		ERROR("crtx_sdbus_match_add failed: %s\n", strerror(-r));
		
		free(track->match.match_str);
		
		return r;
	}
	
	return 0;
}

int crtx_sdbus_track_remove(struct crtx_sdbus_listener *lstnr, struct crtx_sdbus_track *track) {
	if (track->match.match_str) {
		free(track->match.match_str);
		
		crtx_sdbus_match_remove(lstnr, &track->match);
	}
	
	return 0;
}

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

int crtx_sdbus_get_events(sd_bus *bus) {
	int f, result;
	
	result = 0;
	
	f = sd_bus_get_events(bus);
	if (f & POLLIN) {
		result |= EPOLLIN;
		f = f & (~POLLIN);
	}
	if (f & POLLOUT) {
		result |= EPOLLOUT;
		f = f & (~POLLOUT);
	}
	
	if (f) {
		INFO("sdbus poll2epoll: untranslated flag %d\n", f);
	}
	
	return result;
}

static char fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
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
	
	// TODO set epoll flags again everytime?
// 	int new_flags;
// 	new_flags = crtx_sd_bus_get_events(sdlist->bus);
// 	if (payload->event_flags != new_flags) {
// 		printf("%d != %d\n", payload->event_flags, new_flags);
// 		payload->event_flags = new_flags;
// 	}
	
	return r;
}

void crtx_sdbus_shutdown_listener(struct crtx_listener_base *data) {
	struct crtx_sdbus_listener *sdlist;
	
	sdlist = (struct crtx_sdbus_listener*) data;
	
	sd_bus_unref(sdlist->bus);
}

char crtx_sdbus_open_bus(sd_bus **bus, enum crtx_sdbus_type bus_type, char *name) {
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

struct crtx_listener_base *crtx_sdbus_new_listener(void *options) {
	struct crtx_sdbus_listener *sdlist;
	int r;
	
	
	sdlist = (struct crtx_sdbus_listener *) options;
	
	if (!sdlist->bus) {
		if (crtx_sdbus_open_bus(&sdlist->bus, sdlist->bus_type, sdlist->name) < 0)
			return 0;
	}
	
	r = sd_bus_get_unique_name(sdlist->bus, &sdlist->unique_name);
	if (r < 0) {
		ERROR("sd_bus_get_unique_name failed: %s\n", strerror(-r));
		return 0;
	}
	
	sdlist->parent.el_payload.fd = sd_bus_get_fd(sdlist->bus);
	sdlist->parent.el_payload.event_flags = crtx_sdbus_get_events(sdlist->bus);
	sdlist->parent.el_payload.data = sdlist;
	sdlist->parent.el_payload.event_handler = &fd_event_handler;
	sdlist->parent.el_payload.event_handler_name = "sdbus event handler";
	
	sdlist->parent.shutdown = &crtx_sdbus_shutdown_listener;
	
	if (sdlist->connection_signals) {
#if 0 // starting with version 237/238
		r = sd_bus_set_connected_signal(sdlist->bus, 1);
		if (r < 0) {
			ERROR("sd_bus_set_connected_signal returned: %s\n", strerror(-r));
			return 0;
		}
#endif
	}
	
	
	return &sdlist->parent;
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
		if (ret) {
			return 0;
		}
	}
	
	return &default_listeners[sdbus_type];
}

void crtx_sdbus_init() {
}

void crtx_sdbus_finish() {
	int i;
	
	for (i=0; i < CRTX_SDBUS_TYPE_MAX; i++) {
		if (default_listeners[i].bus)
			crtx_free_listener(&default_listeners[i].parent);
	}
}




#ifdef CRTX_TEST
static char sdbus_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	sd_bus_message *m;
	
	m = (sd_bus_message*) crtx_event_get_ptr(event);
	
	crtx_sdbus_print_msg(m);
	
	return 0;
}

struct crtx_sdbus_match matches[] = {
	/* match string, event type, callback fct, callback data, <reserved> */
	{ "type='signal'", "sd_bus/signal", 0, 0, 0 },
	{0},
};

int sdbus_main(int argc, char **argv) {
	struct crtx_sdbus_listener sdlist;
	struct crtx_listener_base *lbase;
	struct crtx_sdbus_match *it;
	char ret;
	
	
	memset(&sdlist, 0, sizeof(struct crtx_sdbus_listener));
	
	sdlist.bus_type = CRTX_SDBUS_TYPE_USER;
	sdlist.connection_signals = 1;
	
	lbase = create_listener("sdbus", &sdlist);
	if (!lbase) {
		ERROR("create_listener(sdbus) failed\n");
		exit(1);
	}
	
	crtx_create_task(lbase->graph, 0, "sdbus_test", sdbus_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (ret) {
		ERROR("starting sdbus listener failed\n");
		return 1;
	}
	
	it = matches;
	while (it && it->match_str) {
		crtx_sdbus_match_add(&sdlist, it);
		it++;
	}
	
	crtx_loop();
	
	return 0;  
}

CRTX_TEST_MAIN(sdbus_main);

#endif
