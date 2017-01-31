#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/netlink.h>

#include "core.h"
#include "uevents.h"

// static char nl2uevents_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
static int nl_route_read_cb(struct crtx_netlink_raw_listener *nl_listener, int fd, void *userdata) {
// 	struct crtx_event_loop_payload *payload;
// 	struct crtx_event *nevent;
// 	struct crtx_uevents_listener *ulist;
// 	struct crtx_netlink_raw_listener *nl_listener;
// 	int r;
	char buf[512];
	
// 	payload = (struct crtx_event_loop_payload*) event->data.raw.pointer;
// 	nl_listener = (struct crtx_netlink_raw_listener*) payload->data;
	
// 	ulist = (struct crtx_uevents_listener*) userdata;
	
// 	frame = (struct can_frame *) malloc(sizeof(struct can_frame));
// 	r = read(clist->sockfd, frame, sizeof(struct can_frame));
// 	if (r != sizeof(struct can_frame)) {
// 		fprintf(stderr, "wrong can frame size: %d\n", r);
// 		return 1;
// 	}
// 	
// 	// 	printf("0x%04x ", frame->can_id);
// 	
// 	nevent = create_event("can", frame, sizeof(frame));
// 	// 	nevent->cb_before_release = &can_event_before_release_cb;
// 	
// 	// 	memcpy(&nevent->data.raw.uint64, &frame.data, sizeof(uint64_t));
// 	
// 	add_event(clist->parent.graph, nevent);
	
	int i, len;
	printf("asd\n");
	len = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
	if (len == -1) {
		ERROR("TODO %s\n", strerror(errno));
		return 0;
	}
	
	i = 0;
	while (i<len) {
		printf("%s\n", buf+i);
		i += strlen(buf+i)+1;
	}
	
	return 1;
}

static void shutdown_uevents_listener(struct crtx_listener_base *lbase) {
	struct crtx_uevents_listener *ulist;
	
	ulist = (struct crtx_uevents_listener*) lbase;
	
	free_listener(&ulist->nl_listener.parent);
	ulist->parent.graph = 0;
}


static char uevents_start_listener(struct crtx_listener_base *listener) {
	struct crtx_uevents_listener *ulist;
	int ret;
	
	ulist = (struct crtx_uevents_listener*) listener;
	
	ret = crtx_start_listener(&ulist->nl_listener.parent);
	if (!ret) {
		ERROR("starting netlink_raw listener failed\n");
		return ret;
	}
	
	return 0;
}

struct crtx_listener_base *crtx_new_uevents_listener(void *options) {
	struct crtx_uevents_listener *ulist;
// 	struct crtx_netlink_raw_listener nl_listener;
	struct crtx_listener_base *lbase;
// 	char ret;
	
	
	ulist = (struct crtx_uevents_listener*) options;
	
	ulist->nl_listener.socket_protocol = NETLINK_KOBJECT_UEVENT;
	ulist->nl_listener.nl_family = AF_NETLINK;
	ulist->nl_listener.nl_groups = -1;
// 	ulist->nl_listener.nlmsg_types = 0;
	
	ulist->nl_listener.read_cb = nl_route_read_cb;
	
// 	ulist->nl_listener.raw2dict = &crtx_netlink_raw_raw2dict_ifaddr;
// 	ulist->nl_listener.all_fields = 1;
	
	lbase = create_listener("netlink_raw", &ulist->nl_listener);
	if (!lbase) {
		ERROR("create_listener(netlink_raw) failed\n");
		exit(1);
	}
	
// 	crtx_create_task(lbase->graph, 0, "nl2uevents_handler", nl2uevents_handler, ulist);
	
// 	new_eventgraph(&ulist->parent.graph, 0, 0);
	ulist->parent.graph = ulist->nl_listener.parent.graph;
	
	ulist->parent.shutdown = &shutdown_uevents_listener;
	ulist->parent.start_listener = &uevents_start_listener;
	
	return &ulist->parent;
}

void crtx_uevents_init() {
}

void crtx_uevents_finish() {
}




#ifdef CRTX_TEST

static char uevents_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	
	
	return 1;
}

int netlink_raw_main(int argc, char **argv) {
	struct crtx_uevents_listener ulist;
	struct crtx_listener_base *lbase;
	int ret;
	
	memset(&ulist, 0, sizeof(struct crtx_uevents_listener));
	
	lbase = create_listener("uevents", &ulist);
	if (!lbase) {
		ERROR("create_listener(uevents) failed\n");
		exit(1);
	}
	
	crtx_create_task(lbase->graph, 0, "uevents_test", uevents_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting uevents listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(netlink_raw_main);
#endif
