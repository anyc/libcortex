/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "intern.h"
#include "core.h"
#include "nl_libnl.h"

// handler that is called on file descriptor events
static char libnl_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct crtx_evloop_fd *payload;
	struct crtx_libnl_listener *libnl_lstnr;
	int r;
	struct crtx_evloop_callback *el_cb;
	
	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
// 	payload = (struct crtx_evloop_fd*) event->data.pointer;
	libnl_lstnr = (struct crtx_libnl_listener *) el_cb->data;
	
	// nl_recvmsgs_default will eventually call the registered callback
	r = nl_recvmsgs_default(libnl_lstnr->sock);
	if (r) {
		fprintf(stderr, "nl_recvmsgs_default failed: %d\n", r);
	}
	
	return 0;
}

static char libnl_start_listener(struct crtx_listener_base *listener) {
	struct crtx_libnl_listener *libnl_lstnr;
	int ret;
	
	
	libnl_lstnr = (struct crtx_libnl_listener*) listener;
	
	ret = nl_connect(libnl_lstnr->sock, NETLINK_ROUTE); 
	if (ret < 0) {
		ERROR("nl_connect failed: %d\n", ret);
		return ret;
	}
	
	libnl_lstnr->base.evloop_fd.fd = nl_socket_get_fd(libnl_lstnr->sock);
	
	return 0;
}

void free_libnl_listener(struct crtx_listener_base *lbase, void *userdata) {
	struct crtx_libnl_listener *libnl_lstnr;
	
	libnl_lstnr = (struct crtx_libnl_listener*) lbase;
	
	nl_socket_free(libnl_lstnr->sock);
}

struct crtx_listener_base *crtx_new_nl_libnl_listener(void *options) {
	struct crtx_libnl_listener *libnl_lstnr;
	struct crtx_libnl_callback *cb;
	int r;
	
	libnl_lstnr = (struct crtx_libnl_listener*) options;
	
	
	libnl_lstnr->sock = nl_socket_alloc();
	if (!libnl_lstnr->sock) {
		fprintf(stderr, "nl_socket_alloc failed\n");
		return 0;
	}
	
	nl_socket_disable_seq_check(libnl_lstnr->sock);
	
	cb = libnl_lstnr->callbacks;
	while (cb->func) {
		r = nl_socket_modify_cb(libnl_lstnr->sock, cb->type, cb->kind, cb->func, cb->arg);
		if (r) {
			fprintf(stderr, "nl_socket_modify_cb failed %d\n", r);
			nl_socket_free(libnl_lstnr->sock);
			return 0;
		}
		cb++;
	}
	
	libnl_lstnr->base.start_listener = &libnl_start_listener;
	
// 	libnl_lstnr->base.evloop_fd.data = libnl_lstnr;
// 	libnl_lstnr->base.evloop_fd.crtx_event_flags = EVLOOP_READ;
// 	libnl_lstnr->base.evloop_fd.event_handler = &libnl_fd_event_handler;
// 	libnl_lstnr->base.evloop_fd.event_handler_name = "libnl fd handler";
	libnl_lstnr->base.free = &free_libnl_listener;
	crtx_evloop_init_listener(&libnl_lstnr->base,
						0,
						EVLOOP_READ,
						0,
						&libnl_fd_event_handler,
						libnl_lstnr,
						0, 0
					);
	
	
	return &libnl_lstnr->base;
}

void crtx_nl_libnl_init() {
}

void crtx_nl_libnl_finish() {
}





#ifdef CRTX_TEST

#include <linux/netlink.h>
#include <netlink/genl/genl.h> // for struct nlmsghdr

#include "nl_route_raw.h" // for crtx_nl_route_raw2dict_interface

static char libnl_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_libnl_listener *libnl;
	int r;
	
	libnl = (struct crtx_libnl_listener *) event->origin;
	
	if (event->data.type == 'u' && event->data.uint32 == CRTX_LSTNR_STARTED) {
		struct rtgenmsg rt_hdr = { .rtgen_family = AF_PACKET, };
		
		r = nl_send_simple(libnl->sock, RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP, &rt_hdr, sizeof(rt_hdr));
		if (r < 0) {
			ERROR("nl_send_simple failed: %d\n", r);
			return 1;
		}
	}
	
	return 0;
}

int crtx_libnl_msg2dict(struct nl_msg *msg, void *arg) {
	struct crtx_dict *dict;
	
	dict = crtx_nl_route_raw2dict_interface(nlmsg_hdr(msg), 1);
	
	crtx_print_dict(dict);
	
	return NL_OK;
}

int crtx_libnl_msg_end(struct nl_msg *msg, void *arg) {
	#ifdef QUERY_ONLY
	crtx_init_shutdown();
	#else
	struct crtx_libnl_listener *libnl_lstnr;
	
	libnl_lstnr = (struct crtx_libnl_listener*) arg;
	
	nl_socket_add_memberships(libnl_lstnr->sock, RTNLGRP_LINK, 0);
	#endif
	
	return NL_OK;
}

struct crtx_libnl_callback libnl_callbacks[] = {
	{ NL_CB_VALID, NL_CB_CUSTOM, crtx_libnl_msg2dict, 0},
	{ NL_CB_FINISH, NL_CB_CUSTOM, crtx_libnl_msg_end, 0},
	{ 0 },
};

int libnl_main(int argc, char **argv) {
	struct crtx_libnl_listener libnl;
	struct crtx_listener_base *lbase;
	struct crtx_libnl_callback *cbit;
	int r;
	
	
	memset(&libnl, 0, sizeof(struct crtx_libnl_listener));
	
	for (cbit = libnl_callbacks; cbit->func; cbit++) {
		cbit->arg = &libnl;
	}
	libnl.callbacks = libnl_callbacks;
	
	lbase = create_listener("nl_libnl", &libnl);
	if (!lbase) {
		ERROR("create_listener(libnl) failed\n");
		exit(1);
	}
	
	lbase->state_graph = lbase->graph;
	crtx_create_task(lbase->graph, 0, "libnl_test", libnl_test_handler, 0);
	
	r = crtx_start_listener(lbase);
	if (r) {
		ERROR("starting libnl listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(libnl_main);
#endif
