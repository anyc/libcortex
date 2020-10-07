#ifndef _CRTX_NETIF_H
#define _CRTX_NETIF_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include "../libnl.h"

struct crtx_netif_listener {
	struct crtx_listener_base base;
	
	struct crtx_libnl_listener libnl_lstnr;
	
	char query_existing; // 1 = query existing interfaces, 2 = query and stop listener
};

struct crtx_listener_base *crtx_setup_netif_listener(void *options);
int crtx_netif_query_interfaces(struct crtx_netif_listener *netif_lstnr);
CRTX_DECLARE_ALLOC_FUNCTION(netif)

void crtx_netif_init();
void crtx_netif_finish();

#ifdef __cplusplus
}
#endif

#endif
