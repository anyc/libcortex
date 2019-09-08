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
	
	ret = crtx_create_listener("curl", &fc_lstnr->curl_lstnr);
	if (ret) {
		ERROR("create_listener(curl) failed\n");
		return;
	}
	
	fc_lstnr->reinit_cb(fc_lstnr->reinit_cb_data);
	
	ret = crtx_start_listener(&fc_lstnr->curl_lstnr.base);
	if (ret) {
		ERROR("starting curl listener failed\n");
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
		ERROR("starting fork listener failed\n");
		return rv;
	}
	
	return 0;
}

struct crtx_listener_base *crtx_new_forked_curl_listener(void *options) {
	struct crtx_forked_curl_listener *fc_lstnr;
	int ret;
	
	
	fc_lstnr = (struct crtx_forked_curl_listener *) options;
	
	fc_lstnr->fork_lstnr.reinit_cb = &reinit_cb;
	fc_lstnr->fork_lstnr.reinit_cb_data = fc_lstnr;
	
	ret = crtx_create_listener("fork", &fc_lstnr->fork_lstnr);
	if (ret) {
		ERROR("create_listener(fork) failed\n");
		return 0;
	}
	
// 	crtx_create_task(fc_lstnr->base.graph, 0, "udev_test", udev_test_handler, fc_lstnr);
	
// 	fc_lstnr->base.shutdown = &shutdown_listener;
	fc_lstnr->base.start_listener = &start_listener;
	
	return &fc_lstnr->base;  
}

void crtx_forked_curl_init() {
}

void crtx_forked_curl_finish() {
}

#else /* CRTX_TEST */

#include <stdint.h>
#include <inttypes.h>

struct crtx_forked_curl_listener fc_lstnr;
size_t last_dlnow = 0;

static char curl_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	if (event->type == CRTX_CURL_ET_FINISHED) {
		printf("curl finished, shutdown now\n");
		
		crtx_init_shutdown();
	}
	
	if (event->type == CRTX_CURL_ET_PROGRESS) {
		int r;
		int64_t dlnow, dltotal;
		
		r = crtx_event_get_value_by_key(event, "dlnow", 'I', &dlnow, sizeof(dlnow));
		if (r < 0) {
			fprintf(stderr, "crtx_event_get_value_by_key() failed: %d\n", r);
			return r;
		}
		r = crtx_event_get_value_by_key(event, "dltotal", 'I', &dltotal, sizeof(dltotal));
		if (r < 0) {
			fprintf(stderr, "crtx_event_get_value_by_key() failed: %d\n", r);
			return r;
		}
		
		if (last_dlnow != dlnow) {
			fprintf(stderr, "progress: %"PRId64" / %"PRId64"\n", dlnow, dltotal);
			last_dlnow = dlnow;
		}
	}
	
	if (event->type == CRTX_CURL_ET_HEADER) {
		int r;
		char *s;
		
		r = crtx_event_get_value_by_key(event, "line", 's', &s, sizeof(s));
		if (r < 0) {
			fprintf(stderr, "crtx_event_get_value_by_key() failed: %d\n", r);
			return r;
		}
		
		printf("H: %s\n", s);
	}
	
	return 0;
}

static void reinit_cb(void *reinit_cb_data) {
	crtx_create_task(fc_lstnr.curl_lstnr.base.graph, 0, "curl_test", &curl_event_handler, 0);
}

static char fork_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	if (event->type == CRTX_FORK_EVT_CHILD_STOPPED) {
		printf("child done, shutdown parent\n");
		
		crtx_init_shutdown();
	} else {
		printf("unexpected event\n");
	}
	
	return 0;
}

int fcurl_main(int argc, char **argv) {
	int ret;
	
	
	memset(&fc_lstnr, 0, sizeof(struct crtx_forked_curl_listener));
	
// 	clist.url = argv[1];
// 	clist.write_fd = stdout;
// 	clist.progress_callback = &crtx_curl_progress_callback;
// 	clist.progress_cb_data = &clist;
// 	
// 	clist.header_callback = &crtx_curl_header_callback;
// 	clist.header_cb_data = &clist;
	
	fc_lstnr.reinit_cb = &reinit_cb;
	fc_lstnr.reinit_cb_data = &fc_lstnr;
	
	fc_lstnr.curl_lstnr.url = argv[1];
	fc_lstnr.curl_lstnr.write_fd = stdout;
	
	ret = crtx_create_listener("forked_curl", &fc_lstnr);
	if (ret) {
		ERROR("create_listener(forked_curl) failed\n");
		return 0;
	}
	
// 	curl_easy_setopt(clist.easy, CURLOPT_NOBODY, 1);
// 	curl_easy_setopt(clist.easy, CURLOPT_VERBOSE, 1L);
	
	crtx_create_task(fc_lstnr.fork_lstnr.base.graph, 0, "fork_event_handler", &fork_event_handler, 0);
	
	ret = crtx_start_listener(&fc_lstnr.base);
	if (ret) {
		ERROR("starting forked_curl listener failed\n");
		return 0;
	}
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(fcurl_main);

#endif
