
#ifndef _CRTX_AVAHI_H
#define _CRTX_AVAHI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "core.h"
#include "sdbus.h"

// not required here but included for user convenience
#include <avahi-common/address.h>

struct crtx_avahi_service {
	int32_t interface;
	int32_t protocol;
	int32_t aprotocol;
	uint32_t flags;
	char *name;
	char *type;
	char *domain;
	char *host;
	char *address;
	uint32_t port;
	void *txt; // TODO
	
	char action;
// 	char *path;
	void *payload;
};

struct crtx_avahi_listener {
	struct crtx_listener_base base;
	
	
	struct crtx_sdbus_listener sdlist;
	struct crtx_listener_base *sdbase;
	
	struct crtx_avahi_service service_browser;
	struct crtx_sdbus_match sdbus_match[3];
};

int crtx_avahi_resolve_service(struct crtx_avahi_service *service);
char crtx_avahi_service(struct crtx_event *event);
char crtx_avahi_publish_service(struct crtx_avahi_service *service);
char crtx_avahi_remove_service(struct crtx_avahi_service *service);
struct crtx_listener_base *crtx_setup_avahi_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(avahi)

void crtx_avahi_init();
void crtx_avahi_finish();

#ifdef __cplusplus
}
#endif

#endif
