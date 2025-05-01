/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "intern.h"
#include "core.h"
#include "netlink_raw.h"

static char netlink_raw_generic_read(struct crtx_event *event, void *userdata, void **sessiondata) {
	int len, r;
	char buffer[4096];
	struct nlmsghdr *nlh;
	struct crtx_netlink_raw_listener *nl_lstnr;
	
	
	nl_lstnr = (struct crtx_netlink_raw_listener *) userdata;
	
	nlh = (struct nlmsghdr *) buffer;
	
	len = recv(nl_lstnr->fd, buffer, sizeof(buffer), 0);
	if (len > 0) {
		while ((NLMSG_OK(nlh, len)) && (nlh->nlmsg_type != NLMSG_DONE)) {
			// does the user want a filtered list of messages?
			if (nl_lstnr->message_types) {
				uint16_t *it;
				
				it = nl_lstnr->message_types;
				while (*it && *it != nlh->nlmsg_type) { it++;}
				
				if (nlh->nlmsg_type == *it) {
					r = nl_lstnr->message_callback(nlh, len, nl_lstnr->message_callback_data);
					if (r < 0) {
						CRTX_DBG("netlink_raw: message_callback returned: %d\n", r);
					}
				}
			} else {
				r = nl_lstnr->message_callback(nlh, len, nl_lstnr->message_callback_data);
				if (r < 0) {
					CRTX_DBG("netlink_raw: message_callback returned: %d\n", r);
				}
			}
			
			nlh = NLMSG_NEXT(nlh, len);
		}
		
		if (nlh->nlmsg_type == NLMSG_DONE) {
			r = nl_lstnr->message_callback(nlh, len, nl_lstnr->message_callback_data);
			if (r < 0) {
				CRTX_DBG("netlink_raw: message_callback returned: %d\n", r);
			}
		}
	}
	
	if (len < 0) { //  && len != EAGAIN
		CRTX_ERROR("netlink_raw recv failed: %s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}

// static void *netlink_raw_tmain(void *data) {
// 	struct crtx_netlink_raw_listener *nl_listener;
// 	int r;
// 	
// 	nl_listener = (struct crtx_netlink_raw_listener*) data;
// 	
// 	r = 0;
// 	while (!nl_listener->stop && r == 0) {
// 		r = nl_listener->read_cb(nl_listener, nl_listener->fd, nl_listener->read_cb_userdata);
// 	}
// 	
// 	return 0;
// }

static char netlink_el_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_netlink_raw_listener *nl_listener;
	struct crtx_evloop_callback *el_cb;
	
	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	
	nl_listener = (struct crtx_netlink_raw_listener*) el_cb->event_handler_data;
	
	nl_listener->read_cb(nl_listener, nl_listener->fd, nl_listener->read_cb_userdata);
	
	return 0;
}

// static void stop_thread(struct crtx_thread *t, void *data) {
// 	struct crtx_netlink_raw_listener *nl_listener;
// 	
// 	CRTX_DBG("stopping netlink_raw listener\n");
// 	
// 	nl_listener = (struct crtx_netlink_raw_listener*) data;
// 	
// 	nl_listener->stop = 1;
// 	
// 	if (nl_listener->fd)
// 		shutdown(nl_listener->fd, SHUT_RDWR);
// }

struct crtx_listener_base *crtx_setup_netlink_raw_listener(void *options) {
	struct crtx_netlink_raw_listener *nl_listener;
	struct sockaddr_nl addr;
	crtx_handle_task_t event_handler;
	
	
	nl_listener = (struct crtx_netlink_raw_listener*) options;
	
	if ((nl_listener->fd = socket(PF_NETLINK, SOCK_RAW, nl_listener->socket_protocol)) == -1) {
		CRTX_ERROR("opening socket failed: %s", strerror(errno));
		return 0;
	}
	
	memset(&addr, 0, sizeof(addr));
	addr.nl_family = nl_listener->nl_family;
	addr.nl_groups = nl_listener->nl_groups;
	
	if (bind(nl_listener->fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		CRTX_ERROR("binding socket failed: %s", strerror(errno));
		return 0;
	}
	
// 	nl_listener->base.thread = get_thread(netlink_raw_tmain, nl_listener, 0);
// 	nl_listener->base.thread->do_stop = &stop_thread;
	
	if (nl_listener->read_cb) {
		event_handler = &netlink_el_event_handler;
	} else {
		event_handler = &netlink_raw_generic_read;
	}
	
	crtx_evloop_init_listener(&nl_listener->base,
						0, nl_listener->fd,
						CRTX_EVLOOP_READ,
						0,
						event_handler,
						nl_listener,
						0,
						0
					);
	
	return &nl_listener->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(netlink_raw)

void crtx_netlink_raw_init() {
}

void crtx_netlink_raw_finish() {
}

// see uevents for an example
