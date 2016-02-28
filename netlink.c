
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>

#include "core.h"
#include "netlink.h"


struct crtx_dict *crtx_netlink_raw2dict_ifaddr(struct crtx_netlink_listener *nl_listener, struct nlmsghdr *nlh) {
	struct crtx_dict *dict;
	struct crtx_dict_item *di, *cdi;
	const char *s;
	size_t slen;
	
	char got_ifname;
	char ifname[IFNAMSIZ];
	char s_addr[INET6_ADDRSTRLEN];
	struct sockaddr_storage addr;
	struct ifaddrmsg *ifa;
	struct rtattr *rth;
	struct ifa_cacheinfo *cinfo;
	int rtl;
	
	
	ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
	rth = IFA_RTA(ifa);
	rtl = IFA_PAYLOAD(nlh);
	
	got_ifname = 0;
	
	dict = crtx_init_dict(0, 0, 0);
	
	// new or deleted address?
	{
		di = crtx_alloc_item(dict);
		
		if (nlh->nlmsg_type == RTM_NEWADDR)
			crtx_fill_data_item(di, 's', "type", "add", 0, DIF_DATA_UNALLOCATED);
		else
		if (nlh->nlmsg_type == RTM_DELADDR)
			crtx_fill_data_item(di, 's', "type", "del", 0, DIF_DATA_UNALLOCATED);
	}
	
	if (nl_listener->all_fields) {
		di = crtx_alloc_item(dict);
		
		switch (ifa->ifa_scope) {
			case RT_SCOPE_UNIVERSE:
				crtx_fill_data_item(di, 's', "scope", "global", 0, DIF_DATA_UNALLOCATED);
				break;
			case RT_SCOPE_SITE:
				crtx_fill_data_item(di, 's', "scope", "site", 0, DIF_DATA_UNALLOCATED);
				break;
			case RT_SCOPE_LINK:
				crtx_fill_data_item(di, 's', "scope", "link", 0, DIF_DATA_UNALLOCATED);
				break;
			case RT_SCOPE_HOST:
				crtx_fill_data_item(di, 's', "scope", "host", 0, DIF_DATA_UNALLOCATED);
				break;
			case RT_SCOPE_NOWHERE:
				crtx_fill_data_item(di, 's', "scope", "nowhere", 0, DIF_DATA_UNALLOCATED);
				break;
		}
	}
	
	while (rtl && RTA_OK(rth, rtl)) { printf("rta %d %d %d %d\n", rth->rta_type, IFA_LOCAL, IFA_LABEL, __IFA_MAX);
		switch (rth->rta_type) {
			case IFA_LABEL:
				di = crtx_alloc_item(dict);
				
				s = (char*) RTA_DATA(rth);
				
				slen = strlen(s);
				crtx_fill_data_item(di, 's', "interface", stracpy(s, &slen), slen, 0);
				got_ifname = 1;
				break;
			
			case IFA_ANYCAST:
			case IFA_BROADCAST:
			case IFA_MULTICAST:
			case IFA_ADDRESS:
			case IFA_LOCAL:
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
						case IFA_MULTICAST: key = "multicast"; break;
						case IFA_ADDRESS: key = "address"; break;
						case IFA_LOCAL: key = "local"; break;
					}
					
					slen = strlen(s);
					crtx_fill_data_item(di, 's', key, stracpy(s, &slen), slen, 0);
				}
				
				break;
			
			case IFA_CACHEINFO:
				if (nl_listener->all_fields) {
					di = crtx_alloc_item(dict);
					di->type = 'D';
					di->key = "cache";
					
					di->ds = crtx_init_dict("uuuu", 4, 0);
					
					cinfo = (struct ifa_cacheinfo*) RTA_DATA(rth);
					
					cdi = crtx_get_first_item(di->ds);
					crtx_fill_data_item(cdi, 'u', "valid_lft", cinfo->ifa_valid, 0, 0);
					cdi = crtx_get_next_item(di->ds, cdi);
					crtx_fill_data_item(cdi, 'u', "preferred_lft", cinfo->ifa_prefered, 0, 0);
					cdi = crtx_get_next_item(di->ds, cdi);
					crtx_fill_data_item(cdi, 'u', "cstamp", cinfo->cstamp, 0, 0);
					cdi = crtx_get_next_item(di->ds, cdi);
					crtx_fill_data_item(cdi, 'u', "tstamp", cinfo->tstamp, 0, 0);
					
					cdi->flags |= DIF_LAST;
				}
				
				// 0xFFFFFFFFUL == forever
				
				break;
			
			case IFA_FLAGS:
				{
					di = crtx_alloc_item(dict);
					di->type = 'D';
					di->key = "flags";
					
					di->ds = crtx_init_dict(0, 0, 0);
					
					unsigned int *flags = (unsigned int*) RTA_DATA(rth);
					
					#define IFA_FLAG(name) \
						if (*flags & IFA_F_ ## name) { \
							cdi = crtx_alloc_item(di->ds); \
							crtx_fill_data_item(cdi, 's', 0, #name, 0, DIF_DATA_UNALLOCATED); \
						}
					
					cdi = 0;
					
					IFA_FLAG(TEMPORARY);
					IFA_FLAG(NODAD);
					IFA_FLAG(OPTIMISTIC);
					IFA_FLAG(DADFAILED);
					IFA_FLAG(HOMEADDRESS);
					IFA_FLAG(DEPRECATED);
					IFA_FLAG(TENTATIVE);
					IFA_FLAG(PERMANENT);
					IFA_FLAG(MANAGETEMPADDR);
					IFA_FLAG(NOPREFIXROUTE);
					IFA_FLAG(MCAUTOJOIN);
					IFA_FLAG(STABLE_PRIVACY);
					
					if (cdi)
						cdi->flags |= DIF_LAST;
				}
				break;
			
			default:
				DBG("netlink: unhandled rta_type %d\n", rth->rta_type);
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
	
	len = recv(nl_listener->sockfd, nlh, 4096, 0);
	while (!nl_listener->stop && len > 0) {
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
					event->data.raw.ds = nl_listener->raw2dict(nl_listener, nlh);
					
					add_event(nl_listener->parent.graph, event);
				}
			}
			
			nlh = NLMSG_NEXT(nlh, len);
		}
		
		len = recv(nl_listener->sockfd, nlh, 4096, 0);
	}
	printf("asd %d\n", len);
	if (len < 0) {
		ERROR("netlink recv failed: %s\n", strerror(errno));
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
	
	memset(&nl_listener, 0, sizeof(struct crtx_netlink_listener));
	
	// setup a netlink listener
	nl_listener.socket_protocol = NETLINK_ROUTE;
	nl_listener.nl_family = AF_NETLINK;
	// e.g., RTMGRP_LINK, RTMGRP_NEIGH, RTMGRP_IPV4_IFADDR, RTMGRP_IPV6_IFADDR
	nl_listener.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
	nl_listener.nlmsg_types = nlmsg_types;
	
	nl_listener.raw2dict = &crtx_netlink_raw2dict_ifaddr;
	nl_listener.all_fields = 1;
	
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
	
	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	req.n.nlmsg_type = RTM_GETADDR;
// 	req.r.ifa_family = AF_INET; // e.g., AF_INET (IPv4-only) or AF_INET6 (IPv6-only)
	
	crtx_netlink_send_req(&nl_listener, &req.n);
	
	// start processing responses
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(netlink_main);
#endif