#ifndef _CRTX_SIP_H
#define _CRTX_SIP_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

struct crtx_sip_listener {
	struct crtx_listener_base base;
	
	struct eXosip_t *ctx;
	
	int transport;
	char *src_addr;
	int src_port;
	int family;
	int secure;
	
	char *username;
	char *userid;
	char *password;
	
	char *dst_addr;
	
	int rid;
};

char crtx_eXosip_event2dict(struct eXosip_event *ptr, struct crtx_dict **dict_ptr);

struct crtx_listener_base *crtx_setup_sip_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(sip)

void crtx_sip_init();
void crtx_sip_finish();

#endif
