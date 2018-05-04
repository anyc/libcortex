#ifndef _CRTX_SIP_H
#define _CRTX_SIP_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

struct crtx_sip_listener {
	struct crtx_listener_base parent;
	
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

struct crtx_listener_base *crtx_new_sip_listener(void *options);
// char crtx_eXosip_event2dict(struct eXosip_event *ptr, struct crtx_dict **dict_ptr);

void crtx_sip_init();
void crtx_sip_finish();

#endif
