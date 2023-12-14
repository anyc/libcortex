/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <inttypes.h>
#include <errno.h>

#include "intern.h"
#include "core.h"
#include "sdbus.h"
#include "threads.h"

void crtx_sdbus_print_msg(sd_bus_message *m, const char *sig) {
	char *val;
	
	if (!sig)
		sig = sd_bus_message_get_signature(m, 1);
	
	printf("sd_bus_message %s %s %s %s %s \"%s\"\n",
		  sd_bus_message_get_path(m),
		  sd_bus_message_get_interface(m),
		  sd_bus_message_get_member(m),
		  sd_bus_message_get_destination(m),
		  sd_bus_message_get_sender(m),
		  sig
	);
	
	while (*sig != 0) {
		if (*sig == 's') {
			sd_bus_message_read_basic(m, 's', &val);
			printf("\tval \"%s\"\n", val);
		}
		
		sig++;
	}
}

int crtx_sdbus_next_to_dict_item(sd_bus_message *msg, struct crtx_dict_item *ditem, char no_reduce) {
	int rv, n_elements;
	char type;
	const char *contents;
	struct crtx_dict_item subitem;
	struct crtx_dict_item *it;
	
	rv = sd_bus_message_peek_type(msg, &type, &contents); CRTX_RET_GEZ(rv)
	if (rv == 0)
		return 0;
	
	// VDBG("next: %c %s\n", type, contents);
	
	n_elements = 0;
	switch (type) {
		case 'e':
		case 'v':
		case 'a': {
			// TODO first item should be length of the array
// 			int array_length;
// 			
// 			if (type == 'a') {
// 				rv = sd_bus_message_read_basic(msg, type, &array_length); CRTX_RET_GEZ(rv)
// 				printf("len %d\n", array_length);
// 			}
			
			rv = sd_bus_message_enter_container(msg, type, contents); CRTX_RET_GEZ(rv)
			
			ditem->type = 'D';
			ditem->dict = (struct crtx_dict*) calloc(1, sizeof(struct crtx_dict));
			
			while (1) {
				rv = sd_bus_message_at_end(msg, 0);
				if (rv == 1)
					break;
				
				memset(&subitem, 0, sizeof(struct crtx_dict_item));
				rv = crtx_sdbus_next_to_dict_item(msg, &subitem, no_reduce);
				if (rv < 0) {
					return rv;
				}
				if (rv < 1) {
					break;
				}
				
				// convert {sv} into a single key=value tuple (= crtx_dict_item) if possible
				if (!no_reduce && !strcmp(contents, "sv") && ditem->dict->signature_length == 1) {
					it = &ditem->dict->items[ditem->dict->signature_length-1];
					
					if (subitem.type == 'D' && subitem.dict->signature_length == 1) {
						struct crtx_dict *olddict;
						
						olddict = ditem->dict;
						
						ditem->key = it->string;
						it->string = 0;
						ditem->flags |= CRTX_DIF_ALLOCATED_KEY;
						ditem->type = subitem.dict->items[0].type;
						ditem->pointer = subitem.dict->items[0].pointer;
						if ((subitem.dict->items[0].flags & CRTX_DIF_DONT_FREE_DATA) == 0)
							subitem.dict->items[0].pointer = 0;
						ditem->size = subitem.dict->items[0].size;
						
						crtx_dict_unref(olddict);
						crtx_free_dict_item_data(&subitem);
					} else {
						it->key = it->string;
						it->flags |= CRTX_DIF_ALLOCATED_KEY;
						it->type = subitem.type;
						it->pointer = subitem.pointer;
						it->size = subitem.size;
					}
					break;
				}
				
				// it = crtx_alloc_item(ditem->dict);
				ditem->dict->signature_length += 1;
				ditem->dict->items = (struct crtx_dict_item*) realloc(ditem->dict->items, sizeof(struct crtx_dict_item)*ditem->dict->signature_length);
				it = &ditem->dict->items[ditem->dict->signature_length-1];
				memcpy(it, &subitem, sizeof(struct crtx_dict_item));
			}
			
			rv = sd_bus_message_exit_container(msg); CRTX_RET_GEZ(rv)
			
			n_elements = 1; break; }
		case 's':
		case 'o': {
			const char *s;
			
			ditem->type = 's';
			rv = sd_bus_message_read_basic(msg, type, &s);
			if (rv < 0)
				return rv;
			
			ditem->string = strdup(s);
			if (type == 'o')
				ditem->key = "object";
			n_elements = 1; break; }
		case 'h':
		case 'b': {
			int i;
			
			i=0;
			ditem->type = 'u';
			rv = sd_bus_message_read_basic(msg, type, &i);
			if (rv < 0)
				return rv;
			
			ditem->uint32 = i;
			if (type == 'h')
				ditem->key = "fd";
			if (type == 'b')
				ditem->key = "bool";
			n_elements = 1; break; }
		case 'q':
		case 'u': {
			uint32_t i;
			
			i=0;
			ditem->type = 'u';
			rv = sd_bus_message_read_basic(msg, type, &i);
			if (rv < 0)
				return rv;
			
			ditem->uint32 = i;
			n_elements = 1; break; }
		case 'n': {
			int16_t i;
			
			ditem->type = 'i';
			rv = sd_bus_message_read_basic(msg, type, &i);
			if (rv < 0)
				return rv;
			
			ditem->int32 = i;
			n_elements = 1; break; }
		case 'i': {
			int32_t i;
			
			i=0;
			ditem->type = 'i';
			rv = sd_bus_message_read_basic(msg, type, &i);
			if (rv < 0)
				return rv;
			
			ditem->int32 = i;
			n_elements = 1; break; }
		case 't': {
			uint64_t i;
			
			ditem->type = 'U';
			rv = sd_bus_message_read_basic(msg, type, &i);
			if (rv < 0)
				return rv;
			
			ditem->uint64 = i;
			n_elements = 1; break; }
		case 'x': {
			int64_t i;
			
			ditem->type = 'I';
			rv = sd_bus_message_read_basic(msg, type, &i);
			if (rv < 0)
				return rv;
			
			ditem->int64 = i;
			n_elements = 1; break; }
		case 'd': {
			double i;
			
			ditem->type = 'd';
			rv = sd_bus_message_read_basic(msg, type, &i);
			if (rv < 0)
				return rv;
			
			ditem->double_fp = i;
			n_elements = 1; break; }
		default:
			CRTX_ERROR("TODO crtx_sdbus_next_to_dict_item(): %c\n", type);
			// rv = sd_bus_message_skip(msg, 0); CRTX_RET_GEZ(rv)
			break;
	}
	
	return n_elements;
}

void crtx_sdbus_trigger_event_processing(struct crtx_sdbus_listener *lstnr) {
	crtx_evloop_trigger_callback(lstnr->base.evloop_fd.evloop, &lstnr->base.default_el_cb);
}

int crtx_sdbus_get_property_async_print_response(sd_bus_message *m,
												 void *userdata,
												 sd_bus_error *ret_error)
{
	int rv;
	const char *contents;
	
	
	rv = sd_bus_message_peek_type(m, NULL, &contents); CRTX_RET_GEZ(rv)
	rv = sd_bus_message_enter_container(m, 'v', contents); CRTX_RET_GEZ(rv)
	
	crtx_sdbus_print_msg(m, contents);
	
	rv = sd_bus_message_exit_container(m); CRTX_RET_GEZ(rv)
	
	return 0;
}

int crtx_sdbus_get_property_async(sd_bus *bus, const char *service, const char *object_path, const char *interface, const char *name, sd_bus_message_handler_t callback, void *userdata, uint64_t usec) {
	int rv;
	sd_bus_message *m;
	
	
	m = 0;
	rv = sd_bus_message_new_method_call(bus,
		&m,
		service,
		object_path,
		"org.freedesktop.DBus.Properties",
		"Get"
		);
	if (rv < 0) {
		fprintf(stderr, "Failed to create method call: %s\n", strerror(-rv));
		return 0;
	}
	
	rv = sd_bus_message_append_basic(m, 's', interface); CRTX_RET_GEZ(rv)
	rv = sd_bus_message_append_basic(m, 's', name); CRTX_RET_GEZ(rv)
	
	sd_bus_call_async(bus, 0,
		m,
		callback,
		userdata,
		usec
		);
	if (rv < 0) {
		fprintf(stderr, "DBus get_property (%s %s %s %s) failed: %s\n",
				service, object_path, interface, name, strerror(-rv));
		return 0;
	}
	
	sd_bus_message_unref(m);
	
	return 0;
}

struct async_callback {
	struct crtx_signals signal;
	
	sd_bus_message **reply;
};

static int async_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	struct async_callback *async_cb_data;
	
	async_cb_data = (struct async_callback*) userdata;
	
	*async_cb_data->reply = m;
	
	sd_bus_message_ref(m);
	
// 	printf("%p\n", ret_error);
// 	error = sd_bus_message_get_error(ret_error);
// 	if (ret_error) {
// 		CRTX_ERROR("DBus error: %s %s\n", ret_error->name, ret_error->message);
// 	}
	
	crtx_send_signal(&async_cb_data->signal, 1);
	
	return 0;
}

/*
 * In applications which have a separate event loop thread, use crtx_sdbus_call()
 * in the worker threads instead of sd_bus_call.
 * 
 * crtx_sdbus_call acts like a synchronous function but, internally, makes an
 * asynchronous DBus call and waits for a signal from the event loop thread afterwards.
 * 
 * This way, only the event loop thread will handle incoming messages and not one of
 * the worker threads.
 */
int crtx_sdbus_call(struct crtx_sdbus_listener *lstnr, sd_bus_message *msg, sd_bus_message **reply, uint64_t timeout_us) {
	struct async_callback async_cb_data;
	sd_bus_slot *slot;
	int err;
	
	
	async_cb_data.reply = reply;
	crtx_init_signal(&async_cb_data.signal);
	
	slot = 0;
	crtx_lock_listener_source(&lstnr->base);
	err = sd_bus_call_async(lstnr->bus, &slot, msg, async_cb, &async_cb_data, timeout_us);
	crtx_unlock_listener_source(&lstnr->base);
	
	if (err < 0) {
		// just pass the error code to the caller
		goto finish;
	}
	
	// this triggers a call to sdbus_process() which sets timeout/flags/etc
	crtx_sdbus_trigger_event_processing(lstnr);
	
	if (lstnr->single_threaded) {
		while (!crtx_signal_is_active(&async_cb_data.signal)) {
			err = crtx_loop_onetime();
			if (err)
				break;
		}
	} else {
		crtx_wait_on_signal(&async_cb_data.signal, 0);
	}
finish:
	crtx_lock_listener_source(&lstnr->base);
	if (slot)
		sd_bus_slot_unref(slot);
	crtx_unlock_listener_source(&lstnr->base);
	
	crtx_shutdown_signal(&async_cb_data.signal);
	
	return err;
}

int crtx_sdbus_get_objects_msg_async(struct crtx_sdbus_listener *lstnr, const char *service, sd_bus_message_handler_t callback, void *userdata, uint64_t timeout_us) {
	int rv;
	sd_bus_message *m;
	
	
	m = 0;
	rv = sd_bus_message_new_method_call(lstnr->bus,
		&m,
		service,
		"/",
		CRTX_DBUS_OBJECT_MANAGER_INTERFACE,
		"GetManagedObjects"
		);
	if (rv < 0) {
		fprintf(stderr, "Failed to create method call: %s\n", strerror(-rv));
		return 0;
	}
	
	// rv = sd_bus_message_append_basic(m, 's', interface); CRTX_RET_GEZ(rv)
	// rv = sd_bus_message_append_basic(m, 's', name); CRTX_RET_GEZ(rv)
	
	sd_bus_call_async(lstnr->bus, 0 /* slot */,
		m,
		callback,
		userdata,
		timeout_us
		);
	if (rv < 0) {
		fprintf(stderr, "DBus get_property (%s %s %s %s) failed: %s\n",
				service, "/", CRTX_DBUS_OBJECT_MANAGER_INTERFACE, "GetManagedObjects", strerror(-rv));
		return 0;
	}
	
	sd_bus_message_unref(m);
	
	return 0;
}

static void match_event_release(struct crtx_event *event, void *userdata) {
	sd_bus_message_unref(event->data.pointer);
}

static int match_event_cb_generic(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	struct crtx_sdbus_match *match;
	struct crtx_event *event;
	
	
	match = (struct crtx_sdbus_match*) userdata;
	
	crtx_create_event(&event);
	event->description = match->event_type;
	
	crtx_event_set_raw_data(event, 'p', m, sizeof(m), CRTX_DIF_DONT_FREE_DATA);
	event->cb_before_release = &match_event_release;
	
	sd_bus_message_ref(m);
	
	crtx_add_event(match->listener->base.graph, event);
	
	return 0;
}

static int match_event_cb_track(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	struct crtx_sdbus_track *track;
	char *peer, *old, *new;
	int r;
	
	
	track = (struct crtx_sdbus_track *) userdata;
	
	r = sd_bus_message_read_basic(m, 's', &peer);
	if (r < 0) {
		CRTX_ERROR("sd_bus_message_read_basic failed: %s\n", strerror(r));
		return r;
	}
	r = sd_bus_message_read_basic(m, 's', &old);
	if (r < 0) {
		CRTX_ERROR("sd_bus_message_read_basic failed: %s\n", strerror(r));
		return r;
	}
	r = sd_bus_message_read_basic(m, 's', &new);
	if (r < 0) {
		CRTX_ERROR("sd_bus_message_read_basic failed: %s\n", strerror(r));
		return r;
	}
	
	if (strcmp(peer, track->peer)) {
		CRTX_ERROR("received unexpected peer: \"%s\" != \"%s\"\n", peer, track->peer);
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
	
	CRTX_DBG("sdbus will listen for \"%s\" (etype: \"%s\")\n", match->match_str, match->event_type);
	
	r = sd_bus_add_match(lstnr->bus, &match->slot, match->match_str, match->callback, match->callback_data);
	if (r < 0) {
		CRTX_ERROR("sd_bus_add_match failed: %s\n", strerror(-r));
		
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
		free(dll);
		
		sd_bus_slot_unref(match->slot);
	} else {
		CRTX_ERROR("did not find match obj in linked list\n");
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
		CRTX_ERROR("crtx_sdbus_match_add failed: %s\n", strerror(-r));
		
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
	
	result = 0; // | CRTX_EVLOOP_EDGE_TRIGGERED;
	
	f = sd_bus_get_events(bus);
	if (f < 0) {
		CRTX_ERROR("sd_bus_get_events() failed: %d\n", f);
		return f;
	}
	if (f & POLLIN) {
		result |= CRTX_EVLOOP_READ;
		f = f & (~POLLIN);
	}
	if (f & POLLOUT) {
		result |= CRTX_EVLOOP_WRITE;
		f = f & (~POLLOUT);
	}
	if (f & POLLERR) {
		CRTX_DBG("error flags from sd_bus_get_events()\n");
		f = f & (~POLLERR);
	}
	
	if (f) {
		CRTX_INFO("sdbus poll2epoll: untranslated flag %d\n", f);
	}
	
	return result;
}

#if 1
void poll_flags2str(unsigned int flags) {
	#define RETFLAG(flag) if (flags & flag) printf(#flag " ");
	
	RETFLAG(POLLIN);
	RETFLAG(POLLOUT);
	RETFLAG(POLLPRI);
// 	RETFLAG(POLLRDHUP);
	RETFLAG(POLLERR);
	RETFLAG(POLLHUP);
	RETFLAG(POLLNVAL);
}
#endif

static char update_evloop_settings(struct crtx_sdbus_listener *sdlist, struct crtx_evloop_callback *el_cb) {
	int new_flags, r;
	uint64_t timeout_us;
	
	
	crtx_lock_listener_source(&sdlist->base);
	
	new_flags = crtx_sdbus_get_events(sdlist->bus);
	if (new_flags < 0) {
		CRTX_ERROR("TODO update_evloop_settings()\n");
		
		crtx_unlock_listener_source(&sdlist->base);
		
		return new_flags;
	}
	
	if (new_flags & POLLHUP) {
		CRTX_VDBG("POLLHUP set by sdbus on fd %d\n", sd_bus_get_fd(sdlist->bus));
	}
	
	el_cb->crtx_event_flags |= CRTX_EVLOOP_TIMEOUT;
	new_flags |= CRTX_EVLOOP_TIMEOUT;
	
	if (el_cb->crtx_event_flags != new_flags) {
		printf("TODO sdbus update_evloop_settings() %d != %d\n", el_cb->crtx_event_flags, new_flags);
// 		poll_flags2str(new_flags);
		// TODO set epoll flags again everytime?
// 		el_cb->crtx_event_flags = new_flags;
// 		crtx_evloop_enable_cb(el_cb->fd_entry->evloop, el_cb);
	}
	
	// NOTE sd_bus_get_timeout() provides an absolute timeout timestamp based on CLOCK_MONOTONIC
	// 18446744073709551615 == UINT64_MAX
	r = sd_bus_get_timeout(sdlist->bus, &timeout_us);
	if (r < 0) {
		CRTX_ERROR("sd_bus_get_timeout failed: %s\n", strerror(-r));
		
		crtx_unlock_listener_source(&sdlist->base);
		
		return r;
	}
	
	// NOTE we use PRId64 although the type is uint64_t to easily see if timeout_us == UINT64_MAX == (uint64_t) -1
	CRTX_VDBG("sdbus timeout: %"PRId64"\n", timeout_us);
	crtx_evloop_set_timeout_abs(el_cb, CLOCK_MONOTONIC, timeout_us);
	
	crtx_unlock_listener_source(&sdlist->base);
	
	return 0;
}

static char fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_sdbus_listener *sdlist;
	int r;
	struct crtx_evloop_callback *el_cb;
	
	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	
	sdlist = (struct crtx_sdbus_listener *) userdata;
	
	while (1) {
		crtx_lock_listener_source(&sdlist->base);
		r = sd_bus_process(sdlist->bus, NULL);
		crtx_unlock_listener_source(&sdlist->base);
		
		if (r < 0) {
			CRTX_ERROR("sd_bus_process failed: %s\n", strerror(-r));
			break;
		}
		if (r == 0)
			break;
	}
	
	update_evloop_settings(sdlist, el_cb);
	
	return r;
}

static void fd_error_handler(struct crtx_evloop_callback *el_cb, void *userdata) {
	struct crtx_sdbus_listener *sdlist;
	int r;
	
	
	CRTX_ERROR("error from sdbus fd\n");
	
	sdlist = (struct crtx_sdbus_listener *) userdata;
	
	while (1) {
		crtx_lock_listener_source(&sdlist->base);
		r = sd_bus_process(sdlist->bus, NULL);
		crtx_unlock_listener_source(&sdlist->base);
		
		if (r < 0) {
			CRTX_ERROR("sd_bus_process failed: %s\n", strerror(-r));
			break;
		}
		if (r == 0)
			break;
	}
	
	// TODO remove fd, etc.
	if (sdlist->error_cb) {
		sdlist->error_cb(sdlist->connected_cb_data);
	}
}

static void shutdown_listener(struct crtx_listener_base *data) {
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
				
				CRTX_DBG("using custom DBUS at address \"%s\"\n", name);
				
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
			CRTX_ERROR("crtx_setup_sdbus_listener\n");
			return -1;
	}
	if (r < 0) {
		fprintf(stderr, "Failed to open bus: %s\n", strerror(-r));
		return r;
	}
	
	return 0;
}


static int connected_match_handler(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	const char *sig;
	char *val;
	struct crtx_sdbus_listener *sdlist;
	
// 	crtx_sdbus_print_msg(m, 0);
	sig = sd_bus_message_get_signature(m, 1);
	
	if (!strcmp(sd_bus_message_get_member(m), "NameAcquired") && sig[0] == 's') {
		sd_bus_message_read_basic(m, 's', &val);
		
		sdlist = (struct crtx_sdbus_listener *) userdata;
		
		if (!strcmp(val, sdlist->unique_name))
			sdlist->connected_cb(m, sdlist->connected_cb_data, 0);
	}
	
	return 0;
}

struct crtx_listener_base *crtx_sdbus_new_listener(void *options) {
	struct crtx_sdbus_listener *sdlist;
	int r;
	
	
	sdlist = (struct crtx_sdbus_listener *) options;
	
	if (!sdlist->bus || !sd_bus_is_open(sdlist->bus)) {
		r = crtx_sdbus_open_bus(&sdlist->bus, sdlist->bus_type, sdlist->name);
		if (r < 0) {
			return 0;
		}
	}
	
	r = sd_bus_get_unique_name(sdlist->bus, &sdlist->unique_name);
	if (r < 0) {
		CRTX_ERROR("sd_bus_get_unique_name failed: %s\n", strerror(-r));
		return 0;
	}
	
	crtx_evloop_init_listener(&sdlist->base,
						sd_bus_get_fd(sdlist->bus),
						crtx_sdbus_get_events(sdlist->bus),
						0,
						&fd_event_handler,
						sdlist,
						&fd_error_handler,
						sdlist
					);
	
	update_evloop_settings(sdlist, &sdlist->base.default_el_cb);
	
	sdlist->base.shutdown = &shutdown_listener;
	// immediately call sdbus_process() once after the event loop starts
	sdlist->base.flags |= CRTX_LSTNR_STARTUP_TRIGGER;
	
	if (sdlist->connection_signals) {
#if 0 // starting with version 237/238
		r = sd_bus_set_connected_signal(sdlist->bus, 1);
		if (r < 0) {
			CRTX_ERROR("sd_bus_set_connected_signal returned: %s\n", strerror(-r));
			return 0;
		}
#else
		// adding ",member='NameAcquired'" to the match string will result in no match for some reason
		sdlist->connected_match.match_str = "type='signal',sender='org.freedesktop.DBus',"
				"path='/org/freedesktop/DBus',interface='org.freedesktop.DBus'";
// 		sdlist->connected_match.match_str = "type='signal'";
		sdlist->connected_match.callback = connected_match_handler;
		sdlist->connected_match.callback_data = sdlist;
		
		crtx_sdbus_match_add(sdlist, &sdlist->connected_match);
#endif
	}
	
	return &sdlist->base;
}

static struct crtx_sdbus_listener default_listeners[CRTX_SDBUS_TYPE_MAX] = { { { { 0 } } } };

struct crtx_sdbus_listener *crtx_sdbus_get_default_listener(enum crtx_sdbus_type sdbus_type) {
	if (!default_listeners[sdbus_type].bus) {
		int ret;
		
		
		default_listeners[sdbus_type].bus_type = sdbus_type;
		
		ret = crtx_setup_listener("sdbus", &default_listeners[sdbus_type]);
		if (ret) {
			CRTX_ERROR("create_listener(timer) failed: %s\n", strerror(-ret));
			return 0;
		}
		
		ret = crtx_start_listener(&default_listeners[sdbus_type].base);
		if (ret) {
			return 0;
		}
	}
	
	return &default_listeners[sdbus_type];
}

CRTX_DEFINE_ALLOC_FUNCTION(sdbus)

void crtx_sdbus_init() {
}

void crtx_sdbus_finish() {
	int i;
	
	for (i=0; i < CRTX_SDBUS_TYPE_MAX; i++) {
		if (default_listeners[i].bus)
			crtx_shutdown_listener(&default_listeners[i].base);
	}
}




#ifdef CRTX_TEST
static char sdbus_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	sd_bus_message *m;
	
	m = (sd_bus_message*) crtx_event_get_ptr(event);
	
	crtx_sdbus_print_msg(m, 0);
	sd_bus_message_rewind(m, 1);
	
	return 0;
}

struct crtx_sdbus_match matches[] = {
	/* match string, event type, callback fct, callback data, <reserved> */
	{ 0, "type='signal'", "sd_bus/signal", 0, 0, 0 },
	{0},
};

int sdbus_main(int argc, char **argv) {
	struct crtx_sdbus_listener sdlist;
	struct crtx_sdbus_match *it;
	int ret;
	
	
	memset(&sdlist, 0, sizeof(struct crtx_sdbus_listener));
	
	sdlist.bus_type = CRTX_SDBUS_TYPE_USER;
	sdlist.connection_signals = 1;
	
	ret = crtx_setup_listener("sdbus", &sdlist);
	if (ret) {
		CRTX_ERROR("create_listener(sdbus) failed: %s\n", strerror(-ret));
		exit(1);
	}
	
	crtx_create_task(sdlist.base.graph, 0, "sdbus_test", sdbus_test_handler, 0);
	
	ret = crtx_start_listener(&sdlist.base);
	if (ret) {
		CRTX_ERROR("starting sdbus listener failed\n");
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
