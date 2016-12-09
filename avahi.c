
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>

#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>

#include "avahi.h"
#include "sd_bus.h"

struct crtx_listener_base *crtx_new_avahi_listener(void *options) {
// 	struct crtx_avahi_listener *epl;
// 	// 	int pipe_fds[2];
// 	struct avahi_event ctrl_event;
// 	int ret;
// 	
// 	
// 	epl = (struct crtx_avahi_listener*) options;
// 	
// 	epl->avahi_fd = avahi_create1(0);
// 	if (epl->avahi_fd < 0) {
// 		ERROR("avahi_create1 failed: %s\n", strerror(errno));
// 		return 0;
// 	}
// 	
// 	new_eventgraph(&epl->parent.graph, "avahi", 0);
// 	
// 	epl->parent.shutdown = &shutdown_avahi_listener;
// 	
// 	if (!epl->no_thread) {
// 		epl->parent.thread = get_thread(crtx_avahi_main, epl, 0);
// 		epl->parent.thread->do_stop = &stop_thread;
// 	} 
// 	// 	else {
// 	// 		if (crtx_root->event_loop.listener)
// 	// 			ERROR("multiple crtx_root->event_loop\n");
// 	// 		
// 	// 		crtx_root->event_loop.listener = epl;
// 	// 	}
// 	
// 	ret = pipe(epl->pipe_fds); // 0 read, 1 write
// 	if (ret < 0) {
// 		ERROR("creating control pipe failed: %s\n", strerror(errno));
// 		// 		epl->control_fd = -1;
// 	} else {
// 		// 		epl->control_fd = pipe_fds[1];
// 		
// 		memset(&ctrl_event, 0, sizeof(struct avahi_event));
// 		ctrl_event.events = EPOLLIN;
// 		ctrl_event.data.ptr = 0;
// 		
// 		crtx_avahi_add_fd_intern(epl, epl->pipe_fds[0], &ctrl_event);
// 	}
// 	
// 	if (epl->max_n_events < 1)
// 		epl->max_n_events = 1;
// 	
// 	// 0 is our default, -1 is no timeout for avahi
// 	epl->timeout -= 1;
// 	
// 	return &epl->parent;
	return 0;
}

void crtx_avahi_init() {
}

void crtx_avahi_finish() {
}


#ifdef CRTX_TEST

// static AvahiSimplePoll *simple_poll = NULL;
// 
// int avahiPollFunc(struct pollfd *ufds, unsigned int nfds, int timeout, void *userdata) {
// 	printf("nfds %d\n", nfds);
// 	
// 	return 0;
// }

int avahi_main(int argc, char **argv) {
	int r;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *reply = NULL;
	struct crtx_sdbus_listener *slist;
	
	slist = crtx_sdbus_get_default_listener(CRTX_SDBUS_TYPE_SYSTEM);
	
	r = sd_bus_message_new_method_call(slist->bus,
									   &m,
									"org.freedesktop.Avahi",  /* service to contact */
									"/", /* object path */
									"org.freedesktop.Avahi.Server",  /* interface name */
									"EntryGroupNew"                          /* method name */
	);
	if (r < 0) {
		fprintf(stderr, "Failed to issue method call: %s %s\n", error.name, error.message);
		return 0;
	}
	
	#define SDCHECK(r) { if (r < 0) {printf("err %d %s %d\n", r, strerror(r), __LINE__); exit(1); } }
	r = sd_bus_call(slist->bus, m, -1, &error, &reply); SDCHECK(r)
	
// 	u_int32_t res;
	char *s;
	r = sd_bus_message_read(reply, "o", &s);
	if (r < 0) {
		fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
	}
	printf("s %s\n", s);
	
	sd_bus_message_unref(m);
	
// 	AvahiClient *client = 0;
// 	int error;
// 	int ret = 1;
// 	struct timeval tv;
// 	
// 	
// 	if (!(simple_poll = avahi_simple_poll_new())) {
// 		fprintf(stderr, "Failed to create simple poll object.\n");
// 		goto fail;
// 	}
// 	
// 	avahi_simple_poll_set_func(simple_poll, &avahiPollFunc, 0);
// 	
// // 	name = avahi_strdup("MegaPrinter");
// 	
// 	client = avahi_client_new(avahi_simple_poll_get(simple_poll), 0, 0, NULL, &error);
// 	
// 	if (!client) {
// 		fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
// 		goto fail;
// 	}
// 	
// // 	/* After 10s do some weird modification to the service */
// // 	avahi_simple_poll_get(simple_poll)->timeout_new(
// // 		avahi_simple_poll_get(simple_poll),
// // 													avahi_elapse_time(&tv, 1000*10, 0),
// // 													modify_callback,
// // 												 client);
// 	
// 	
// 	avahi_simple_poll_loop(simple_poll);
// 	
// 	ret = 0;
// 	
// fail:
// 	/* Cleanup things */
// 	if (client)
// 		avahi_client_free(client);
// 	if (simple_poll)
// 		avahi_simple_poll_free(simple_poll);
// // 	avahi_free(name);
// 	
// 	return ret;
	
// 	struct crtx_avahi_listener epl;
// 	struct crtx_listener_base *lbase;
// 	char ret;
// 	struct avahi_event event;
// 	struct crtx_event_loop_payload payload;
// 	
// 	memset(&epl, 0, sizeof(struct crtx_avahi_listener));
// 	
// 	if (argc > 1) {
// 		epl.no_thread = 1;
// 	}
// 	
// 	lbase = create_listener("avahi", &epl);
// 	if (!lbase) {
// 		ERROR("create_listener(avahi) failed\n");
// 		exit(1);
// 	}
// 	
// 	ret = pipe(testpipe); // 0 read, 1 write
// 	if (ret < 0) {
// 		ERROR("creating test pipe failed: %s\n", strerror(errno));
// 		return 1;
// 	}
// 	
// 	memset(&payload, 0, sizeof(struct crtx_event_loop_payload));
// 	payload.fd = testpipe[0];
// 	
// 	event.events = EPOLLIN;
// 	// 	event.data.fd = fd;
// 	event.data.ptr = &payload;
// 	crtx_avahi_add_fd_intern(&epl, testpipe[0], &event);
// 	
// 	crtx_create_task(lbase->graph, 0, "avahi_test", avahi_test_handler, 0);
// 	
// 	ret = crtx_start_listener(lbase);
// 	if (!ret) {
// 		ERROR("starting avahi listener failed\n");
// 		return 1;
// 	}
// 	
// 	if (argc > 1) {
// 		start_timer();
// 		
// 		crtx_loop();
// 		
// 		free_listener(blist);
// 	} else {
// 		struct avahi_control_pipe ecp;
// 		
// 		sleep(1);
// 		write(testpipe[1], "success", 7);
// 		
// 		sleep(1);
// 		
// 		ecp.graph = 0;
// 		write(epl.pipe_fds[1], &ecp, sizeof(struct avahi_control_pipe));
// 	}
// 	
// 	free_listener(lbase);
// 	
// 	close(testpipe[0]);
// 	
// 	return 0;
}

CRTX_TEST_MAIN(avahi_main);
#endif
