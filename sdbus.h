
#ifndef _CRTX_SDBUS_H
#define _CRTX_SDBUS_H

#include <systemd/sd-bus.h>

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
	char *match_str;
	char *event_type;
	struct crtx_sdbus_listener *listener;
};

struct crtx_sdbus_listener {
	struct crtx_listener_base parent;
	
	enum crtx_sdbus_type bus_type;
	char *name; // contains either the host name or machine name if bus_type is
				// either CRTX_SDBUS_TYPE_SYSTEM_REMOTE or CRTX_SDBUS_TYPE_SYSTEM_MACHINE
	sd_bus *bus;
	
	struct crtx_sdbus_match *matches;
};

extern sd_bus *sd_bus_main_bus;

int crtx_sd_bus_message_read_string(sd_bus_message *m, char **p);
char crtx_open_sdbus(sd_bus **bus, enum crtx_sdbus_type bus_type, char *name);
void sd_bus_add_signal_listener(sd_bus *bus, char *path, char *event_type);
void sd_bus_print_event_task(struct crtx_event *event, void *userdata);
void sd_bus_print_msg(sd_bus_message *m);

int crtx_sdbus_get_property_str(struct crtx_sdbus_listener *sdlist, 
								char *destination,
								char *path,
								char *interface,
								char *member,
								char **s);

struct crtx_sdbus_listener *crtx_sdbus_get_default_listener(enum crtx_sdbus_type sdbus_type);

struct crtx_listener_base *crtx_new_sdbus_listener(void *options);

void crtx_sdbus_init();
void crtx_sdbus_finish();

#endif
