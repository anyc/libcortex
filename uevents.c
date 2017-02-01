#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/netlink.h>

#include "core.h"
#include "uevents.h"

static int nl_route_read_cb(struct crtx_netlink_raw_listener *nl_listener, int fd, void *userdata) {
	char buf[512];
	
	int i, len;
	printf("asd\n");
	len = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
	if (len == -1) {
		if (errno == EWOULDBLOCK)
			return;
		
		ERROR("TODO %s\n", strerror(errno));
		return;
	}
	
	i = 0;
	while (i<len) {
		printf("\"\"\"%s\"\"\"\n", buf+i);
		i += strlen(buf+i)+1;
	}
	
	
	
	
// 	struct nlmsghdr *nlh = NULL;
// 	struct sockaddr_nl dest_addr;
// 	struct iovec iov;
// 	struct msghdr msg;
// 	
// 	memset(&dest_addr, 0, sizeof(dest_addr));
// 	
// 	#define MAX_PAYLOAD 1024
// 	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
// 	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
// 	
// 	iov.iov_base = (void *)nlh;
// 	iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);
// 	msg.msg_name = (void *)&dest_addr;
// 	msg.msg_namelen = sizeof(dest_addr);
// 	msg.msg_iov = &iov;
// 	msg.msg_iovlen = 1;
// 	
// 	printf("Waiting for message from kernel\n");
// 	
// 	/* Read message from kernel */
// 	recvmsg(fd, &msg, 0);
// 	
// 	printf(" Received message payload: group %d \"\"\"%s\"\"\"\n", dest_addr.nl_groups, (char*) NLMSG_DATA(nlh));
	
	
	
	
// 	int len;
// 	char buffer[4096];
// // 	struct nlmsghdr *nlh;
// 	
// 	nlh = (struct nlmsghdr *) buffer;
// 	
// 	len = recv(fd, nlh, 4096, 0);
// 	if (len > 0) {
// 		printf("rec %d %d\n", NLMSG_OK(nlh, len), nlh->nlmsg_type != NLMSG_DONE);
// 		while ((NLMSG_OK(nlh, len)) && (nlh->nlmsg_type != NLMSG_DONE)) {
// // 			uint16_t *it;
// 			
// 			printf(" Received message payload: %s\n", (char*) NLMSG_DATA(nlh));
// 			
// // 			it = nlr_list->nlmsg_types;
// // 			while (*it && *it != nlh->nlmsg_type) { it++; }
// // 			
// // 			if (nlh->nlmsg_type == *it) {
// // 				event = create_event(0, 0, 0);
// // 				
// // 				if (!nlr_list->raw2dict) {
// // 					// we don't know the size of data, hence we cannot copy it
// // 					// and we have to wait until all tasks have finished
// // 					
// // 					event->data.raw.pointer = nlh;
// // 					event->data.raw.type = 'p';
// // 					event->data.raw.flags = DIF_DATA_UNALLOCATED;
// // 					
// // 					reference_event_release(event);
// // 					
// // 					add_event(nl_listener->parent.graph, event);
// // 					
// // 					wait_on_event(event);
// // 					
// // 					dereference_event_release(event);
// // 				} else {
// // 					// we copy the data and we do not have to wait
// // 					
// // 					// 					event->data.raw.type = 'D';
// // 					event->data.dict = nlr_list->raw2dict(nlr_list, nlh);
// // 					
// // 					add_event(nl_listener->parent.graph, event);
// // 				}
// // 			}
// 			
// 			nlh = NLMSG_NEXT(nlh, len);
// 		}
// 	}
	
	return 0;
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
	struct crtx_listener_base *lbase;
	
	
	ulist = (struct crtx_uevents_listener*) options;
	
	ulist->nl_listener.socket_protocol = NETLINK_KOBJECT_UEVENT;
	ulist->nl_listener.nl_family = AF_NETLINK;
	ulist->nl_listener.nl_groups = 1;
	
	ulist->nl_listener.read_cb = nl_route_read_cb;
	
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
