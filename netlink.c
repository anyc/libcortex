
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>

#include "core.h"
#include "netlink.h"


struct crtx_dict *crtx_netlink_raw2dict_ifaddr(struct nlmsghdr *nlh) {
	struct ifaddrmsg *ifa;
	struct rtattr *rth;
	int rtl;
	struct crtx_dict *dict;
	struct crtx_dict_item *di;
	const char *s;
	size_t slen;
	
	char got_ifname;
	char ifname[IFNAMSIZ];
	struct sockaddr_storage addr;
	char s_addr[INET6_ADDRSTRLEN];
	
	ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
	rth = IFA_RTA(ifa);
	rtl = IFA_PAYLOAD(nlh);
	
	got_ifname = 0;
	
	dict = crtx_init_dict(0, 0, 0);
	
// 	entry = crtx_alloc_item(dict);
// 	entry = crtx_init_dict(0, 0, 0);
// 	entry->type = 'D';
// 	entry->ds = crtx_init_dict(0, 1, 0);
	
	
// 	di = crtx_get_first_item(dict);
	di = crtx_alloc_item(dict);
	
	if (nlh->nlmsg_type == RTM_NEWADDR)
		crtx_fill_data_item(di, 's', "type", "add", 0, DIF_DATA_UNALLOCATED);
	else
	if (nlh->nlmsg_type == RTM_DELADDR)
		crtx_fill_data_item(di, 's', "type", "del", 0, DIF_DATA_UNALLOCATED);
	
	
	
	while (rtl && RTA_OK(rth, rtl)) { printf("rta %d %d %d %d\n", rth->rta_type, IFA_LOCAL, IFA_LABEL, IFA_ADDRESS);
		switch (rth->rta_type) {
			case IFA_LABEL:
				s = (char*) RTA_DATA(rth);
				
				slen = strlen(s);
				crtx_fill_data_item(di, 's', "interface", stracpy(s, &slen), slen, 0);
				got_ifname = 1;
				break;
			
			case IFA_ANYCAST:
			case IFA_BROADCAST:
			case IFA_ADDRESS:
			case IFA_LOCAL:
	// 			di = crtx_get_next_item(entry->ds, di);
				di = crtx_alloc_item(dict);
				
				memset(&addr, 0, sizeof(addr));
				addr.ss_family = ifa->ifa_family;
				if (addr.ss_family == AF_INET) {
					
					struct sockaddr_in *sin;
					
					sin = (struct sockaddr_in *) &addr;
					memcpy(&sin->sin_addr, RTA_DATA(rth), sizeof(sin->sin_addr));
					s = inet_ntop(ifa->ifa_family, &(((struct sockaddr_in *)&addr)->sin_addr), s_addr, sizeof(s_addr));
					
					if (!s) {
						ERROR("inet_ntop(v4) failed: %s\n", strerror(errno));
					}
				} else
				if (addr.ss_family == AF_INET6) {
					struct sockaddr_in6 *sin6;
					
					sin6 = (struct sockaddr_in6 *) &addr;
					memcpy(&sin6->sin6_addr, RTA_DATA(rth), sizeof(sin6->sin6_addr));
					s = inet_ntop(ifa->ifa_family, &(((struct sockaddr_in6 *)&addr)->sin6_addr), s_addr, sizeof(s_addr));
					
					if (!s) {
						ERROR("inet_ntop(v6) failed: %s\n", strerror(errno));
					}
				} else {
					ERROR("unknown family: %d\n", addr.ss_family);
					s = 0;
				}
				
				if (s) {
					char *key;
					
					switch (rth->rta_type) {
						case IFA_ANYCAST: key = "anycast"; break;
						case IFA_BROADCAST: key = "broadcast"; break;
						case IFA_ADDRESS: key = "address"; break;
						case IFA_LOCAL: key = "local"; break;
					}
					
					slen = strlen(s);
					crtx_fill_data_item(di, 's', key, stracpy(s, &slen), slen, 0);
				}
				
				break;
		}
		rth = RTA_NEXT(rth, rtl);
	}
	
	if (!got_ifname) {
		di = crtx_alloc_item(dict);
		
		if_indextoname(ifa->ifa_index, ifname);
		slen = strlen(ifname);
		crtx_fill_data_item(di, 's', "interface", stracpy(ifname, &slen), slen, 0);
	}
	
	di->flags |= DIF_LAST;
	
	return dict;
}

char crtx_netlink_send_req(struct crtx_netlink_listener *nl_listener, struct nlmsghdr *n) {
	int ret;
	
	ret = send(nl_listener->sockfd, n, n->nlmsg_len, 0);
	if (ret < 0) {
		ERROR("error while sending netlink request: %s\n", strerror(errno));
		return 0;
	}
	
	return 1;
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
	
	struct {
		struct nlmsghdr n;
		struct ifaddrmsg r;
	} req;
// 	struct rtattr *rta;
	
	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	req.n.nlmsg_type = RTM_GETADDR;
// 	req.r.ifa_family = AF_INET;
// 
// 	rta = (struct rtattr *)(((char *)&req) + NLMSG_ALIGN(req.n.nlmsg_len));
// 	rta->rta_len = RTA_LENGTH(16);
	
	crtx_netlink_send_req(&nl_listener, &req.n);
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(netlink_main);
#endif