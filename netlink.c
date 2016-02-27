
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

#include "core.h"
#include "netlink.h"


struct crtx_dict *crtx_netlink_raw2dict_ifaddr(struct nlmsghdr *nlh) {
	struct ifaddrmsg *ifa;
	struct rtattr *rth;
	int rtl;
	struct crtx_dict *dict;
	struct crtx_dict_item *entry, *di;
	
	ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
	rth = IFA_RTA(ifa);
	rtl = IFA_PAYLOAD(nlh);
	
	dict = crtx_init_dict(0, 0, 0);
	
	while (rtl && RTA_OK(rth, rtl)) {
		if (rth->rta_type == IFA_LOCAL) {
			struct in_addr ip_addr;
			char ifname[IFNAMSIZ];
			size_t slen;
			char *s;
			
			entry = crtx_alloc_item(dict);
			entry->type = 'D';
			entry->ds = crtx_init_dict(0, 3, 0);
			
			di = crtx_get_first_item(entry->ds);
			
			if (nlh->nlmsg_type == RTM_NEWADDR)
				crtx_fill_data_item(di, 's', "type", "add", 0, DIF_DATA_UNALLOCATED);
			else
				if (nlh->nlmsg_type == RTM_DELADDR)
					crtx_fill_data_item(di, 's', "type", "del", 0, DIF_DATA_UNALLOCATED);
				
				
				di = crtx_get_next_item(entry->ds, di);
			
			if_indextoname(ifa->ifa_index, ifname);
			slen = strlen(ifname);
			crtx_fill_data_item(di, 's', "interface", stracpy(ifname, &slen), slen, 0);
			
			
			di = crtx_get_next_item(entry->ds, di);
			
			ip_addr.s_addr = (*((uint32_t *)RTA_DATA(rth)));
			s = inet_ntoa(ip_addr);
			
			slen = strlen(s);
			crtx_fill_data_item(di, 's', "ip", stracpy(s, &slen), slen, 0);
			
			
			di->flags |= DIF_LAST;
		}
		rth = RTA_NEXT(rth, rtl);
	}
	
	return dict;
}

static void *netlink_tmain(void *data) {
	int len;
	char buffer[4096];
	struct nlmsghdr *nlh;
	struct crtx_netlink_listener *nl_listener;
	struct crtx_event *event;
	
	nl_listener = (struct crtx_netlink_listener*) data;
	
	nlh = (struct nlmsghdr *) buffer;
	
	while (!nl_listener->stop && (len = recv(nl_listener->sockfd, nlh, 4096, 0)) > 0 ) {
		while ((NLMSG_OK(nlh, len)) && (nlh->nlmsg_type != NLMSG_DONE)) {
			uint16_t *it;
			
			it = nl_listener->nlmsg_types;
			while (*it && *it != nlh->nlmsg_type) { it++; }
			
			if (nlh->nlmsg_type == *it) {
				event = create_event(0, 0, 0);
				
				if (!nl_listener->raw2dict) {
					// we don't know the size of data, hence we cannot copy it
					// and we have to wait until all tasks have finished
					
					event->data.raw.pointer = nlh;
					event->data.raw.type = 'p';
					event->data.raw.flags = DIF_DATA_UNALLOCATED;
					
					reference_event_release(event);
					
					add_event(nl_listener->parent.graph, event);
					
					wait_on_event(event);
					
					dereference_event_release(event);
				} else {
					// we copy the data and we do not have to wait
					
					event->data.raw.type = 'D';
					event->data.raw.ds = nl_listener->raw2dict(nlh);
					
					add_event(nl_listener->parent.graph, event);
				}
			}
			
			nlh = NLMSG_NEXT(nlh, len);
		}
	}
	
	return 0;
}

static void stop_thread(struct crtx_thread *t, void *data) {
	struct crtx_netlink_listener *nl_listener;
	
	DBG("stopping socket listener\n");
	
	nl_listener = (struct crtx_netlink_listener*) data;
	
	nl_listener->stop = 1;
	if (nl_listener->sockfd)
		shutdown(nl_listener->sockfd, SHUT_RDWR);
}

void free_netlink_listener(struct crtx_listener_base *data) {
}

struct crtx_listener_base *crtx_new_netlink_listener(void *options) {
	struct crtx_netlink_listener *nl_listener;
	struct sockaddr_nl addr;
	
	nl_listener = (struct crtx_netlink_listener*) options;
	
	if ((nl_listener->sockfd = socket(PF_NETLINK, SOCK_RAW, nl_listener->socket_protocol)) == -1) {
		perror("couldn't open NETLINK_ROUTE socket");
		return 0;
	}
	
	memset(&addr, 0, sizeof(addr));
	addr.nl_family = nl_listener->nl_family;
	addr.nl_groups = nl_listener->nl_groups;
	
	if (bind(nl_listener->sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("couldn't bind");
		return 0;
	}
	
	new_eventgraph(&nl_listener->parent.graph, 0, 0);
	
	nl_listener->parent.start_listener = 0;
	nl_listener->parent.thread = get_thread(netlink_tmain, nl_listener, 0);
	nl_listener->parent.thread->do_stop = &stop_thread;
	
	return &nl_listener->parent;
}

void crtx_netlink_init() {
}

void crtx_netlink_finish() {
}


#ifdef CRTX_TEST
static uint16_t nlmsg_types[] = {RTM_NEWADDR, RTM_DELADDR, 0};

static char netlink_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	crtx_print_dict(event->data.raw.ds);
	
	return 1;
}

int netlink_main(int argc, char **argv) {
	struct crtx_netlink_listener nl_listener;
	struct crtx_listener_base *lbase;
	char ret;
	
	nl_listener.socket_protocol = NETLINK_ROUTE;
	nl_listener.nl_family = AF_NETLINK;
	nl_listener.nl_groups = RTMGRP_IPV4_IFADDR;
	nl_listener.nlmsg_types = nlmsg_types;
	
	nl_listener.raw2dict = &crtx_netlink_raw2dict_ifaddr;
	
	lbase = create_listener("netlink", &nl_listener);
	if (!lbase) {
		ERROR("create_listener(netlink) failed\n");
		exit(1);
	}
	
	crtx_create_task(lbase->graph, 0, "netlink_test", netlink_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting netlink listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(netlink_main);
#endif