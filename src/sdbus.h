#ifndef _CRTX_SDBUS_H
#define _CRTX_SDBUS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <systemd/sd-bus.h>

#include "linkedlist.h"

#define CRTX_DBUS_OBJECT_MANAGER_INTERFACE "org.freedesktop.DBus.ObjectManager"

enum crtx_sdbus_type {
	CRTX_SDBUS_TYPE_DEFAULT = 0,
	CRTX_SDBUS_TYPE_USER,
	CRTX_SDBUS_TYPE_SYSTEM,
	CRTX_SDBUS_TYPE_SYSTEM_REMOTE,
	CRTX_SDBUS_TYPE_SYSTEM_MACHINE,
	CRTX_SDBUS_TYPE_CUSTOM,
	CRTX_SDBUS_TYPE_MAX,
};

struct crtx_sdbus_listener;

struct crtx_sdbus_match {
	sd_bus_slot *slot;
	
	char *match_str;
	char *event_type;
	
	sd_bus_message_handler_t callback;
	void *callback_data;
	
	struct crtx_sdbus_listener *listener;
};

struct crtx_sdbus_track {
	struct crtx_sdbus_match match;
	
	const char *peer;
	
	void (*appear_cb)(struct crtx_sdbus_track *track, char *peer, char *new_name, void *userdata);
	void (*disappear_cb)(struct crtx_sdbus_track *track, char *peer, char *new_name, void *userdata);
	void *userdata;
};

struct crtx_sdbus_listener {
	struct crtx_listener_base base;
	
	enum crtx_sdbus_type bus_type;
	char *name;	// contains either the host name or machine name if bus_type is
				// either CRTX_SDBUS_TYPE_SYSTEM_REMOTE or CRTX_SDBUS_TYPE_SYSTEM_MACHINE
	sd_bus *bus;
	int connection_signals;
	struct crtx_sdbus_match connected_match;
	sd_bus_message_handler_t connected_cb;
	void *connected_cb_data;
	
	char single_threaded;
	unsigned int process_max_per_run; // break the sd_bus_process loop if number of processed events exceeds this value (0 means no limit)
	
	const char *unique_name;
	
	struct crtx_dll *matches;
	
	void (*error_cb)(void *connected_cb_data);
};

void crtx_sdbus_trigger_event_processing(struct crtx_sdbus_listener *lstnr);
int crtx_sdbus_call(struct crtx_sdbus_listener *lstnr, sd_bus_message *msg, sd_bus_message **reply, uint64_t timeout_us);
int crtx_sdbus_get_property_async(sd_bus *bus, const char *service, const char *object_path, const char *interface, const char *name, sd_bus_message_handler_t callback, void *userdata, uint64_t usec);
int crtx_sdbus_get_property_async_print_response(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

int crtx_sd_bus_message_read_string(sd_bus_message *m, char **p);
char crtx_sdbus_open_bus(sd_bus **bus, enum crtx_sdbus_type bus_type, char *name);
void crtx_sdbus_print_msg(sd_bus_message *m, const char *sig);
int crtx_sdbus_next_to_dict_item(sd_bus_message *msg, struct crtx_dict_item *ditem, char no_reduce);

int crtx_sdbus_match_add(struct crtx_sdbus_listener *lstnr, struct crtx_sdbus_match *match);
int crtx_sdbus_match_remove(struct crtx_sdbus_listener *lstnr, struct crtx_sdbus_match *match);
int crtx_sdbus_track_add(struct crtx_sdbus_listener *lstnr, struct crtx_sdbus_track *track);
int crtx_sdbus_track_remove(struct crtx_sdbus_listener *lstnr, struct crtx_sdbus_track *track);

struct crtx_sdbus_listener *crtx_sdbus_get_default_listener(enum crtx_sdbus_type sdbus_type);
int crtx_sdbus_get_objects_msg_async(struct crtx_sdbus_listener *lstnr, const char *service, sd_bus_message_handler_t callback, void *userdata, uint64_t timeout_us);
int crtx_sdbus_get_objects(struct crtx_sdbus_listener *lstnr, const char *service, struct crtx_dict_item **objects);

struct crtx_listener_base *crtx_sdbus_new_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(sdbus)

void crtx_sdbus_init();
void crtx_sdbus_finish();

#ifdef __cplusplus
}
#endif

#endif
