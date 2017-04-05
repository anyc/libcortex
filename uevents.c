#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/netlink.h>

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
		ERROR("wrong format of uevent msg\n");
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
			ERROR("wrong format of uevent msg\n");
// 			crtx_free_dict(dict);
			crtx_dict_unref(dict);
			return 0;
		}
		
		len = sep - s;
		key = (char*) malloc(len + 1);
		memcpy(key, s, len);
		key[len] = 0;

		len = strlen(sep+1);
		
		di = crtx_alloc_item(dict);
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
	
	
	ulist = (struct crtx_uevents_listener*) userdata;
	
	len = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
	if (len <= -1) {
		if (errno == EWOULDBLOCK)
			return 0;
		
		ERROR("error while receiving nl route event: %s\n", strerror(errno));
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
	
	event = crtx_create_event(0, 0, 0);
	
// 	event->data.string = crtx_stracpy(buf, &ulen);
	event->data.string = (char *) malloc(ulen+1);
	memcpy(event->data.string, buf, ulen);
	event->data.string[ulen] = 0;
	event->data.size = ulen + 1;
	event->data.type = 's';
// 	event->data.flags = CRTX_DIF_DONT_FREE_DATA;
	
// 	reference_event_release(event);
	
	add_event(ulist->parent.graph, event);
	
// 	wait_on_event(event);
	
// 	dereference_event_release(event);
	
	return 0;
}

static void shutdown_uevents_listener(struct crtx_listener_base *lbase) {
	struct crtx_uevents_listener *ulist;
	
	ulist = (struct crtx_uevents_listener*) lbase;
	
	crtx_free_listener(&ulist->nl_listener.parent);
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
	struct crtx_listener_base *lbase;
	
	
	ulist = (struct crtx_uevents_listener*) options;
	
	ulist->nl_listener.socket_protocol = NETLINK_KOBJECT_UEVENT;
	ulist->nl_listener.nl_family = AF_NETLINK;
	ulist->nl_listener.nl_groups = 1;
	
	ulist->nl_listener.read_cb = uevents_read_cb;
	ulist->nl_listener.read_cb_userdata = ulist;
	
	lbase = create_listener("netlink_raw", &ulist->nl_listener);
	if (!lbase) {
		ERROR("create_listener(netlink_raw) failed\n");
		exit(1);
	}
	
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
	char *buf;
	int i;
	struct crtx_dict *dict;
	
	
	buf = event->data.string;
	
	printf("RAW:\n");
	i = 0;
	while (i<event->data.size) {
		printf("\"\"\"%s\"\"\"\n", buf+i);
		i += strlen(buf+i)+1;
	}
	
	dict = crtx_uevents_raw2dict(event);
	
	crtx_print_dict(dict);
	
// 	crtx_free_dict(dict);
	crtx_dict_unref(dict);
	
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
	
// 	free_listener(lbase);
	
	return 0;
}

CRTX_TEST_MAIN(netlink_raw_main);
#endif
