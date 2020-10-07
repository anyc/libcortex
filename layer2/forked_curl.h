#ifndef _CRTX_FORKED_CURL_H
#define _CRTX_FORKED_CURL_H

#ifdef __cplusplus
extern "C" {
#endif

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

struct crtx_listener_base *crtx_setup_forked_curl_listener(void *options);
CRTX_DECLARE_ALLOC_FUNCTION(forked_curl)

void crtx_forked_curl_init();
void crtx_forked_curl_finish();

#ifdef __cplusplus
}
#endif

#endif
