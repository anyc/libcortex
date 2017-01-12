
#ifndef _CRTX_AVAHI_H
#define _CRTX_AVAHI_H

#include "core.h"
#include "sd_bus.h"

// not required here but included for user convenience
#include <avahi-common/address.h>

struct crtx_avahi_service {
	int32_t interface;
	int32_t protocol;
	uint32_t flags;
	char *name;
	char *type;
	char *domain;
	char *host;
	uint32_t port;
// 	char *txt; // TODO
	
	char action;
// 	char *path;
	void *payload;
};

struct crtx_avahi_listener {
	struct crtx_listener_base parent;
	
	
	struct crtx_sdbus_listener sdlist;
	struct crtx_listener_base *sdbase;
	
	struct crtx_avahi_service service_browser;
	struct crtx_sdbus_match sdbus_match[3];
};

char crtx_avahi_service(struct crtx_event *event);
struct crtx_listener_base *crtx_new_avahi_listener(void *options);
void crtx_avahi_init();
void crtx_avahi_finish();

#endif
