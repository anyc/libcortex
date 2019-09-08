#ifndef _CRTX_FORKED_CURL_H
#define _CRTX_FORKED_CURL_H

/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include "fork.h"
#include "curl.h"

struct crtx_forked_curl_listener {
	struct crtx_listener_base base;
	
	struct crtx_fork_listener fork_lstnr;
	struct crtx_curl_listener curl_lstnr;
	
	void (*reinit_cb)(void *cb_data);
	void *reinit_cb_data;
};

struct crtx_listener_base *crtx_new_forked_curl_listener(void *options);

void crtx_forked_curl_init();
void crtx_forked_curl_finish();

#endif
