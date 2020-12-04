/*
 * Mario Kicherer (dev@kicherer.org) 2019
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intern.h"
#include "core.h"
#include "forked_curl.h"

#ifndef CRTX_TEST

static void reinit_cb(void *reinit_cb_data) {
	struct crtx_forked_curl_listener *fc_lstnr;
	int ret;
	
	
	fc_lstnr = (struct crtx_forked_curl_listener *) reinit_cb_data;
	
	crtx_handle_std_signals();
	
	ret = crtx_setup_listener("curl", &fc_lstnr->curl_lstnr);
	if (ret) {
		CRTX_ERROR("create_listener(curl) failed\n");
		return;
	}
	
	if (fc_lstnr->reinit_cb)
		fc_lstnr->reinit_cb(fc_lstnr->reinit_cb_data);
	
	ret = crtx_start_listener(&fc_lstnr->curl_lstnr.base);
	if (ret) {
		CRTX_ERROR("starting curl listener failed\n");
		return;
	}
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_forked_curl_listener *fc_lstnr;
	int rv;
	
	
	fc_lstnr = (struct crtx_forked_curl_listener *) listener;

	fc_lstnr->base.mode = CRTX_NO_PROCESSING_MODE;
	
	rv = crtx_start_listener(&fc_lstnr->fork_lstnr.base);
	if (rv) {
		CRTX_ERROR("starting fork listener failed\n");
		return rv;
	}
	
	return 0;
}

struct crtx_listener_base *crtx_setup_forked_curl_listener(void *options) {
	struct crtx_forked_curl_listener *fc_lstnr;
	int ret;
	
	
	fc_lstnr = (struct crtx_forked_curl_listener *) options;
	
	fc_lstnr->fork_lstnr.reinit_cb = &reinit_cb;
	fc_lstnr->fork_lstnr.reinit_cb_data = fc_lstnr;
	
	fc_lstnr->fork_lstnr.base.graph = fc_lstnr->base.graph;
	
	ret = crtx_setup_listener("fork", &fc_lstnr->fork_lstnr);
	if (ret) {
		CRTX_ERROR("create_listener(fork) failed\n");
		return 0;
	}
	
// 	fc_lstnr->base.shutdown = &shutdown_listener;
	fc_lstnr->base.start_listener = &start_listener;
	
	return &fc_lstnr->base;  
}

CRTX_DEFINE_ALLOC_FUNCTION(forked_curl)

void crtx_forked_curl_init() {
}

void crtx_forked_curl_finish() {
}

#else /* CRTX_TEST */

#include <stdint.h>
#include <inttypes.h>

struct crtx_forked_curl_listener fc_lstnr;
size_t last_dlnow = 0;

static char fork_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	if (event->type == CRTX_FORK_ET_CHILD_STOPPED) {
		printf("child done with rv %d, shutdown parent\n", event->data.int32);
		
		crtx_init_shutdown();
		
		return 0;
	}
	
	if (event->type == CRTX_CURL_ET_FINISHED) {
		printf("curl finished, shutdown child now\n");
		
		crtx_init_shutdown();
		
		return 0;
	}
	
	CRTX_ERROR("unexpected event: %d\n", event->type);
	
	return 0;
}

static void reinit_cb(void *reinit_cb_data) {
	crtx_create_task(fc_lstnr.curl_lstnr.base.graph, 0, "fork_event_handler", &fork_event_handler, 0);
}

int fcurl_main(int argc, char **argv) {
	int ret;
	
	
	memset(&fc_lstnr, 0, sizeof(struct crtx_forked_curl_listener));
	
	fc_lstnr.reinit_cb = &reinit_cb;
	fc_lstnr.reinit_cb_data = &fc_lstnr;
	
	fc_lstnr.curl_lstnr.url = argv[1];
	fc_lstnr.curl_lstnr.write_file_ptr = stdout;
	
	ret = crtx_setup_listener("forked_curl", &fc_lstnr);
	if (ret) {
		CRTX_ERROR("create_listener(forked_curl) failed\n");
		return 0;
	}
	
	crtx_create_task(fc_lstnr.base.graph, 0, "fork_event_handler", &fork_event_handler, 0);
	
	ret = crtx_start_listener(&fc_lstnr.base);
	if (ret) {
		CRTX_ERROR("starting forked_curl listener failed\n");
		return 0;
	}
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(fcurl_main);

#endif
