#ifndef _CRTX_LIBVIRT_H
#define _CRTX_LIBVIRT_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <libvirt/libvirt.h>

struct crtx_libvirt_listener {
	struct crtx_listener_base base;
	
	char *hypervisor;
	
	virConnectPtr conn;
	
	int domain_event_id;
};

struct crtx_listener_base *crtx_setup_libvirt_listener(void *options);
virConnectPtr crtx_libvirt_get_conn(struct crtx_listener_base *base);
CRTX_DECLARE_ALLOC_FUNCTION(libvirt)

void crtx_libvirt_init();
void crtx_libvirt_finish();

#endif
