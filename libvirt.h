
#ifndef _CRTX_LIBVIRT_H
#define _CRTX_LIBVIRT_H

#include <libvirt/libvirt.h>

struct crtx_libvirt_listener {
	struct crtx_listener_base parent;
	
	char *hypervisor;
	
	virConnectPtr conn;
	
	int domain_event_id;
};

struct crtx_listener_base *crtx_new_libvirt_listener(void *options);

void crtx_libvirt_init();
void crtx_libvirt_finish();

#endif
