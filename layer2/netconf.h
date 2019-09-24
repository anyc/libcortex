#ifndef _CRTX_NETCONF_H
#define _CRTX_NETCONF_H

/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include "../nl_libnl.h"

#define CRTX_NETCONF_INTF (1<<0)
#define CRTX_NETCONF_IPV4_ADDR (1<<1)
#define CRTX_NETCONF_IPV6_ADDR (1<<2)

struct crtx_netconf_listener {
	struct crtx_listener_base base;
	
	struct crtx_libnl_listener libnl_lstnr;
	
	int monitor_types;
	char query_existing; // 1 = query existing interfaces, 2 = query and stop listener
	int queried_types;
	
	struct crtx_libnl_callback libnl_callbacks[3];
};

struct crtx_listener_base *crtx_new_netconf_listener(void *options);
int crtx_netconf_query_interfaces(struct crtx_netconf_listener *netconf_lstnr);

void crtx_netconf_init();
void crtx_netconf_finish();

#endif