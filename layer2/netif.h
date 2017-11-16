
#ifndef _CRTX_NETIF_H
#define _CRTX_NETIF_H

#include "../nl_libnl.h"

struct crtx_netif_listener {
	struct crtx_listener_base parent;
	
	struct crtx_libnl_listener libnl_lstnr;
	
	char query_existing; // 1 = query existing interfaces, 2 = query and stop listener
};

struct crtx_listener_base *crtx_new_netif_listener(void *options);
int crtx_netif_query_interfaces(struct crtx_netif_listener *netif_lstnr);

void crtx_netif_init();
void crtx_netif_finish();

#endif
