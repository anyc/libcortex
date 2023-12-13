/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/netlink.h>

#include "intern.h"
#include "core.h"
#include "uevents.h"

struct crtx_dict *crtx_uevents_raw2dict(struct crtx_event *event) {
	char *s, *sep, *key;
	struct crtx_dict *dict;
	struct crtx_dict_item *di;
	size_t len;
	char *buf;
	size_t size;
	
	
	buf = (char *) event->data.string;
	size = event->data.size;
	
	// safety measure
	buf[size-1] = 0;
	
	sep = strchr(buf, '@');
	if (!sep) {
		CRTX_ERROR("wrong format of uevent msg\n");
		return 0;
	}
	s = buf + strlen(buf)+1;
	
	dict = crtx_init_dict(0, 0, 0);
	
	while (s < buf + size) {
		sep = strchr(s, '=');
		if (!sep) {
			len = strlen(s);
			s += len + 1;
			continue;
		}
		if (sep >= buf+size) {
			CRTX_ERROR("wrong format of uevent msg\n");
// 			crtx_free_dict(dict);
			crtx_dict_unref(dict);
			return 0;
		}
		
		len = sep - s;
		key = (char*) malloc(len + 1);
		memcpy(key, s, len);
		key[len] = 0;

		len = strlen(sep+1);
		
		di = crtx_dict_alloc_next_item(dict);
		crtx_fill_data_item(di, 's', key, crtx_stracpy(sep+1, &len), len, CRTX_DIF_ALLOCATED_KEY);
		
		s = sep + 1 + len + 1;
	}
	
	return dict;
}

static int uevents_read_cb(struct crtx_netlink_raw_listener *nl_listener, int fd, void *userdata) {
	char buf[512];
	ssize_t len;
	size_t ulen;
	struct crtx_event *event;
	struct crtx_uevents_listener *ulist;
	char *s;
	
	
	ulist = (struct crtx_uevents_listener*) userdata;
	
	len = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
	if (len <= -1) {
		if (errno == EWOULDBLOCK)
			return 0;
		
		CRTX_ERROR("error while receiving nl route event: %s\n", strerror(errno));
		return errno;
	}
	
	// len is >= 0 here
	ulen = len;
	
// 	int i;
// 	i = 0;
// 	while (i<len) {
// 		printf("\"\"\"%s\"\"\"\n", buf+i);
// 		i += strlen(buf+i)+1;
// 	}
	
	crtx_create_event(&event);
	
	s = (char *) malloc(ulen+1);
	memcpy(s, buf, ulen);
	s[ulen] = 0;
	
	crtx_event_set_raw_data(event, 's', s, ulen, 0);
	
	crtx_add_event(ulist->base.graph, event);
	
	return 0;
}

static void shutdown_uevents_listener(struct crtx_listener_base *lbase) {
	struct crtx_uevents_listener *ulist;
	
	ulist = (struct crtx_uevents_listener*) lbase;
	
	crtx_shutdown_listener(&ulist->nl_listener.base);
	ulist->base.graph = 0;
}


static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_uevents_listener *ulist;
	int ret;
	
	ulist = (struct crtx_uevents_listener*) listener;
	
	ret = crtx_start_listener(&ulist->nl_listener.base);
	if (ret) {
		CRTX_ERROR("starting netlink_raw listener failed\n");
		return ret;
	}
	
	return 0;
}

struct crtx_listener_base *crtx_setup_uevents_listener(void *options) {
	struct crtx_uevents_listener *ulist;
	int r;
	
	
	ulist = (struct crtx_uevents_listener*) options;
	
	ulist->nl_listener.socket_protocol = NETLINK_KOBJECT_UEVENT;
	ulist->nl_listener.nl_family = AF_NETLINK;
	ulist->nl_listener.nl_groups = 1;
	
	ulist->nl_listener.read_cb = uevents_read_cb;
	ulist->nl_listener.read_cb_userdata = ulist;
	
	ulist->base.mode = CRTX_NO_PROCESSING_MODE;
	
	r = crtx_setup_listener("netlink_raw", &ulist->nl_listener);
	if (r) {
		CRTX_ERROR("create_listener(netlink_raw) failed: %s\n", strerror(-r));
		exit(1);
	}
	
	ulist->base.graph = ulist->nl_listener.base.graph;
	ulist->base.shutdown = &shutdown_uevents_listener;
	ulist->base.start_listener = &start_listener;
	
	return &ulist->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(uevents)

void crtx_uevents_init() {
}

void crtx_uevents_finish() {
}




#ifdef CRTX_TEST
struct crtx_uevents_listener ulist;

static char uevents_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	char *buf;
	int i;
	struct crtx_dict *dict;
	
	
	buf = event->data.string;
	
	if (0) {
		printf("RAW:\n");
		i = 0;
		while (i<event->data.size) {
			printf("%s\n", buf+i);
			i += strlen(buf+i)+1;
		}
	}
	
	dict = crtx_uevents_raw2dict(event);
	
	crtx_print_dict(dict);
	
// 	crtx_free_dict(dict);
	crtx_dict_unref(dict);
	
	return 1;
}

int netlink_raw_main(int argc, char **argv) {
	int ret;
	
	
	memset(&ulist, 0, sizeof(struct crtx_uevents_listener));
	
	ret = crtx_setup_listener("uevents", &ulist);
	if (ret) {
		CRTX_ERROR("create_listener(uevents) failed: %s\n", strerror(ret));
		exit(1);
	}
	
	crtx_create_task(ulist.base.graph, 0, "uevents_test", uevents_test_handler, 0);
	
	ret = crtx_start_listener(&ulist.base);
	if (ret) {
		CRTX_ERROR("starting uevents listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	// 	free_listener(&ulist.base);
	
	return 0;
}

CRTX_TEST_MAIN(netlink_raw_main);
#endif
