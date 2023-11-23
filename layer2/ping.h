#ifndef _CRTX_PING_H
#define _CRTX_PING_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2023
 *
 */

#include <netinet/in.h>
#include <linux/icmp.h>
#include <time.h>

#include "../core.h"
#include "../timer.h"

#define CRTX_EVENT_TYPE_FAMILY_PING (600)
#define CRTX_PING_ET_PING_SUCCESS		(CRTX_EVENT_TYPE_FAMILY_PING + 1)
#define CRTX_PING_ET_PING_UNREACHABLE	(CRTX_EVENT_TYPE_FAMILY_PING + 2)
#define CRTX_PING_ET_PING_ERROR			(CRTX_EVENT_TYPE_FAMILY_PING + 3)
#define CRTX_PING_ET_PING_TIMEOUT		(CRTX_EVENT_TYPE_FAMILY_PING + 4)

enum icmp_type { ICMP_T_ECHO, ICMP_T_GATEWAY, ICMP_T_FRAG, ICMP_T_RESERVED};

struct crtx_ping_listener {
	struct crtx_listener_base base;
	
	const char *ip;
	uint16_t echo_id;
	
	uint16_t packets_sent;
	struct timespec sent_ts;
	double last_rt_time;
	char generate_timeout;
	
	struct sockaddr_in dst;
	struct crtx_timer_listener timer_lstnr;
	
	unsigned char *packet;
	
	void (*send_hook)(struct crtx_ping_listener *ping_lstnr, struct icmphdr *data, size_t size, void *userdata);
	int (*recv_hook)(struct crtx_ping_listener *ping_lstnr, int event_type, struct icmphdr *data, size_t size, void *userdata);
	void *userdata;
};

struct crtx_listener_base *crtx_setup_ping_listener(void *options);
int crtx_ping_query_interfaces(struct crtx_ping_listener *ping_lstnr);
CRTX_DECLARE_ALLOC_FUNCTION(ping)

struct crtx_dict *crtx_ping_raw2dict_addr(struct icmphdr *ptr, enum icmp_type icmp_type);

void crtx_ping_init();
void crtx_ping_finish();

#ifdef __cplusplus
}
#endif

#endif

