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
#include "libnl.h"

// handler that is called on file descriptor events
static int libnl_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_libnl_listener *libnl_lstnr;
	int r;
	
	
	libnl_lstnr = (struct crtx_libnl_listener *) userdata;
	
	// nl_recvmsgs_default will eventually call the registered callback
	r = nl_recvmsgs_default(libnl_lstnr->sock);
	if (r) {
		CRTX_ERROR("nl_recvmsgs_default failed: %d\n", r);
	}
	
	return 0;
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_libnl_listener *libnl_lstnr;
	int ret;
	
	
	libnl_lstnr = (struct crtx_libnl_listener*) listener;
	
	ret = nl_connect(libnl_lstnr->sock, NETLINK_ROUTE);
	if (ret < 0) {
		CRTX_ERROR("nl_connect failed: %d\n", ret);
		return ret;
	}
	
	libnl_lstnr->base.evloop_fd.fd = &libnl_lstnr->base.evloop_fd.l_fd;
	libnl_lstnr->base.evloop_fd.l_fd = nl_socket_get_fd(libnl_lstnr->sock);
	
	return 0;
}

void free_libnl_listener(struct crtx_listener_base *lbase, void *userdata) {
	struct crtx_libnl_listener *libnl_lstnr;
	
	libnl_lstnr = (struct crtx_libnl_listener*) lbase;
	
	nl_socket_free(libnl_lstnr->sock);
}

struct crtx_listener_base *crtx_setup_libnl_listener(void *options) {
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
			CRTX_ERROR("nl_socket_modify_cb failed %d\n", r);
			nl_socket_free(libnl_lstnr->sock);
			return 0;
		}
		cb++;
	}
	
	libnl_lstnr->base.start_listener = &start_listener;
	libnl_lstnr->base.free_cb = &free_libnl_listener;
	
	crtx_evloop_init_listener(&libnl_lstnr->base,
						0, 0,
						CRTX_EVLOOP_READ,
						0,
						&libnl_fd_event_handler,
						libnl_lstnr,
						0, 0
					);
	
	
	return &libnl_lstnr->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(libnl)

void crtx_libnl_init() {
}

void crtx_libnl_finish() {
}
