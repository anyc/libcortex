
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>

// for sleep()
#include <unistd.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>

#include "core.h"
#include "netlink.h"


static struct crtx_dict *crtx_netlink_raw2dict_addr(struct crtx_netlink_listener *nl_listener, struct nlmsghdr *nlh) {
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
		
		
		
		di = crtx_alloc_item(dict);
		di->type = 'D';
		di->key = "flags";
		
		di->ds = crtx_init_dict(0, 0, 0);
		
		
		#define IFA_FLAG(name) \
		if (ifa->ifa_flags & IFA_F_ ## name) { \
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
	
	while (rtl && RTA_OK(rth, rtl)) {
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
				// we get this from ifa->ifa_flags
				
// 				if (nl_listener->all_fields) {
// 					unsigned int *flags;
// 					
// 					di = crtx_alloc_item(dict);
// 					di->type = 'D';
// 					di->key = "flags";
// 					
// 					di->ds = crtx_init_dict(0, 0, 0);
// 					
// 					flags = (unsigned int*) RTA_DATA(rth);
// 					
// 					#define IFA_FLAG(name)
// 						if (*flags & IFA_F_ ## name) {
// 							cdi = crtx_alloc_item(di->ds);
// 							crtx_fill_data_item(cdi, 's', 0, #name, 0, DIF_DATA_UNALLOCATED);
// 						}
// 					
// 					cdi = 0;
// 					
// 					IFA_FLAG(TEMPORARY);
// 					IFA_FLAG(NODAD);
// 					IFA_FLAG(OPTIMISTIC);
// 					IFA_FLAG(DADFAILED);
// 					IFA_FLAG(HOMEADDRESS);
// 					IFA_FLAG(DEPRECATED);
// 					IFA_FLAG(TENTATIVE);
// 					IFA_FLAG(PERMANENT);
// 					IFA_FLAG(MANAGETEMPADDR);
// 					IFA_FLAG(NOPREFIXROUTE);
// 					IFA_FLAG(MCAUTOJOIN);
// 					IFA_FLAG(STABLE_PRIVACY);
// 					
// 					if (cdi)
// 						cdi->flags |= DIF_LAST;
// 				}
				break;
			
			default:
				VDBG("netlink: unhandled rta_type %d\n", rth->rta_type);
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

struct crtx_dict *crtx_netlink_raw2dict_if(struct crtx_netlink_listener *nl_listener, struct nlmsghdr *nlh) {
	struct ifinfomsg *ifi;
	struct rtattr *rth;
	int32_t len;
	struct crtx_dict *dict;
	struct crtx_dict_item *di, *cdi;
	char *s;
	char *key;
	size_t slen;
	unsigned char *b;
	struct rtnl_link_stats *stats;
	
	
// 	struct nlmsgerr *nlme;
// 	if (nh->nlmsg_type == NLMSG_ERROR) {
// 		nlme = NLMSG_DATA(nh);
// 		fprintf(stderr, "Got error %d.\n", nlme->error);
// 		return -1;
// 	}
	
	ifi = NLMSG_DATA(nlh);
	rth = IFLA_RTA(ifi);
	len = IFLA_PAYLOAD(nlh);
	
	dict = crtx_init_dict(0, 0, 0);
	
	// new or deleted interface?
	{
		di = crtx_alloc_item(dict);
		
		if (nlh->nlmsg_type == RTM_NEWLINK)
			crtx_fill_data_item(di, 's', "type", "add", 0, DIF_DATA_UNALLOCATED);
		else
		if (nlh->nlmsg_type == RTM_DELLINK)
			crtx_fill_data_item(di, 's', "type", "del", 0, DIF_DATA_UNALLOCATED);
	}
	
	{
		di = crtx_alloc_item(dict);
		di->type = 'D';
		di->key = "flags";
		di->ds = crtx_init_dict(0, 0, 0);

		#define IFI_FLAG(name) \
			if (ifi->ifi_flags & IFF_ ## name) { \
				cdi = crtx_alloc_item(di->ds); \
				crtx_fill_data_item(cdi, 's', 0, #name, 0, DIF_DATA_UNALLOCATED); \
			}
		
		IFI_FLAG(UP);
		IFI_FLAG(BROADCAST);
		IFI_FLAG(DEBUG);
		IFI_FLAG(LOOPBACK);
		IFI_FLAG(POINTOPOINT);
		IFI_FLAG(NOTRAILERS);
		IFI_FLAG(RUNNING);
		IFI_FLAG(NOARP);
		IFI_FLAG(PROMISC);
		IFI_FLAG(ALLMULTI);
		IFI_FLAG(MASTER);
		IFI_FLAG(SLAVE);
		IFI_FLAG(MULTICAST);
		IFI_FLAG(PORTSEL);
		IFI_FLAG(AUTOMEDIA);
		IFI_FLAG(DYNAMIC);
// 		IFI_FLAG(LOWER_UP);
// 		IFI_FLAG(DORMANT);
// 		IFI_FLAG(ECHO);
	}
	
	while (len && RTA_OK(rth, len)) {
		switch (rth->rta_type) {
			case IFLA_ADDRESS:
			case IFLA_BROADCAST:
				di = crtx_alloc_item(dict);
				
				if (RTA_PAYLOAD(rth) != 6)
					ERROR("netlink: RTA_PAYLOAD %zu != 6\n", RTA_PAYLOAD(rth));
				
				s = (char*) malloc(RTA_PAYLOAD(rth)*3);
				b = (unsigned char*) RTA_DATA(rth);
				
				snprintf(s, RTA_PAYLOAD(rth)*3, "%02x:%02x:%02x:%02x:%02x:%02x", 
					    b[0], b[1], b[2], b[3], b[4], b[5]);
				
				switch (rth->rta_type) {
					case IFLA_ADDRESS: key = "address"; break;
					case IFLA_BROADCAST: key = "broadcast"; break;
				}
				crtx_fill_data_item(di, 's', key, s, RTA_PAYLOAD(rth)*3-1, 0);
				
				break;
				
			case IFLA_IFNAME:
				di = crtx_alloc_item(dict);
				
				s = (char*) RTA_DATA(rth);
				
				slen = strlen(s);
				crtx_fill_data_item(di, 's', "interface", stracpy(s, &slen), slen, 0);
				break;
				
			case IFLA_MTU:
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 'u', "mtu", *((unsigned int*) RTA_DATA(rth)), 0, 0);
				break;
				
			case IFLA_LINK:
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 'i', "link", *((int*) RTA_DATA(rth)), 0, 0);
				break;
			
			case IFLA_QDISC:
				s = (char*) RTA_DATA(rth);
				
				slen = strlen(s);
				
				di = crtx_alloc_item(dict);
				crtx_fill_data_item(di, 's', "qdisc", stracpy(s, &slen), slen, 0);
				break;
			
			case IFLA_STATS:
				if (nl_listener->all_fields) {
					di = crtx_alloc_item(dict);
					di->type = 'D';
					di->key = "stats";
					di->ds = crtx_init_dict(0, 23, 0);
					
					stats = (struct rtnl_link_stats *) RTA_DATA(rth);
					
					#define ADD_ITEM(attr) cdi = crtx_get_next_item(di->ds, cdi); \
						crtx_fill_data_item(cdi, 'u', #attr, stats->attr, 0, 0);
					
					cdi = crtx_get_first_item(di->ds);
					crtx_fill_data_item(cdi, 'u', "tx_packets", stats->tx_packets, 0, 0);
					
					
					ADD_ITEM(tx_bytes);
					ADD_ITEM(rx_packets);
					ADD_ITEM(rx_bytes);
					ADD_ITEM(rx_errors);
					ADD_ITEM(tx_errors);
					ADD_ITEM(rx_dropped);
					ADD_ITEM(tx_dropped);
					ADD_ITEM(multicast);
					ADD_ITEM(collisions);
					
					ADD_ITEM(rx_length_errors);
					ADD_ITEM(rx_over_errors);
					ADD_ITEM(rx_crc_errors);
					ADD_ITEM(rx_frame_errors);
					ADD_ITEM(rx_fifo_errors);
					ADD_ITEM(rx_missed_errors);
					
					ADD_ITEM(tx_aborted_errors);
					ADD_ITEM(tx_carrier_errors);
					ADD_ITEM(tx_fifo_errors);
					ADD_ITEM(tx_heartbeat_errors);
					ADD_ITEM(tx_window_errors);
					
					ADD_ITEM(rx_compressed);
					ADD_ITEM(tx_compressed);
				}
				break;
			case IFLA_LINKINFO:
				// TODO (e.g. for VLAN)
				// https://lists.open-mesh.org/pipermail/b.a.t.m.a.n/2014-January/011236.html
				break;
			default:
				VDBG("netlink: unhandled rta_type %d\n", rth->rta_type);
		}
		
		rth = RTA_NEXT(rth, len);
	}
	
	di->flags |= DIF_LAST;
	
	return dict;
}

struct crtx_dict *crtx_netlink_raw2dict_ifaddr(struct crtx_netlink_listener *nl_listener, struct nlmsghdr *nlh) {
	switch (nlh->nlmsg_type) {
		case RTM_NEWADDR:
		case RTM_DELADDR:
			return crtx_netlink_raw2dict_addr(nl_listener, nlh);
			break;
		case RTM_NEWLINK:
		case RTM_DELLINK:
			return crtx_netlink_raw2dict_if(nl_listener, nlh);
			break;
	}
	
	return 0;
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
	printf("recv1\n");
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
					
// 					event->data.raw.type = 'D';
					event->data.dict = nl_listener->raw2dict(nl_listener, nlh);
					
					add_event(nl_listener->parent.graph, event);
				}
			}
			
			nlh = NLMSG_NEXT(nlh, len);
		}
		
		if (nlh->nlmsg_type == NLMSG_DONE)
			send_signal(&nl_listener->msg_done, 0);
		printf("recv2\n");
		if (!nl_listener->stop)
			len = recv(nl_listener->sockfd, nlh, 4096, 0);
	}
	
	if (len < 0) {
		ERROR("netlink recv failed: %s\n", strerror(errno));
	}
	
	return 0;
}

static void stop_thread(struct crtx_thread *t, void *data) {
	struct crtx_netlink_listener *nl_listener;
	
	DBG("stopping netlink listener\n");
	
	nl_listener = (struct crtx_netlink_listener*) data;
	
	nl_listener->stop = 1;
	
	if (nl_listener->sockfd)
		shutdown(nl_listener->sockfd, SHUT_RDWR);
	
	crtx_threads_interrupt_thread(nl_listener->parent.thread);
	
// 	wait_on_signal(&nl_listener->parent.thread->finished);
// 	dereference_signal(&nl_listener->parent.thread->finished);
}

void free_netlink_listener(struct crtx_listener_base *lbase) {
	struct crtx_netlink_listener *nl_listener;
	
	nl_listener = (struct crtx_netlink_listener *) lbase;
	
	stop_thread(lbase->thread, nl_listener);
	
	free_signal(&nl_listener->msg_done);
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
	
	init_signal(&nl_listener->msg_done);
	
	nl_listener->parent.free = &free_netlink_listener;
	nl_listener->parent.start_listener = 0;
	nl_listener->parent.thread = get_thread(netlink_tmain, nl_listener, 0);
	nl_listener->parent.thread->do_stop = &stop_thread;
	reference_signal(&nl_listener->parent.thread->finished);
	
	return &nl_listener->parent;
}

void crtx_netlink_init() {
}

void crtx_netlink_finish() {
}


#ifdef CRTX_TEST
static uint16_t nlmsg_types[] = {RTM_NEWLINK, RTM_DELLINK, RTM_NEWADDR, RTM_DELADDR, 0};

struct {
	struct nlmsghdr n;
	struct ifaddrmsg r;
} addr_req;

struct {
	struct nlmsghdr n;
	struct ifinfomsg ifinfomsg;
} if_req;

static char netlink_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	crtx_print_dict(event->data.dict);
	
	return 1;
}

#include "cache.h"
static uint16_t nlmsg_addr_types[] = {RTM_NEWADDR, RTM_DELADDR, 0};
static char crtx_get_own_ip_addresses_keygen(struct crtx_event *event, struct crtx_dict_item *key) {
	struct crtx_dict_item *di;
	
	if (!event->data.dict)
		event->data.raw_to_dict(&event->data);
	
	di = crtx_get_item(event->data.dict, "address");
	
	key->key = 0;
	key->type = 's';
	key->string = stracpy(di->string, 0);
	if (!key->string)
		return 0;
	key->flags |= DIF_KEY_ALLOCATED;
	
	return 1;
}

char crtx_get_own_ip_addresses() {
	struct crtx_netlink_listener nl_listener;
	struct crtx_listener_base *lbase;
	char ret;
	struct crtx_task *ip_cache_task;
	struct crtx_cache_task *ctask;
	#define TASK2CACHETASK(task) ((struct crtx_cache_task*) (task)->userdata)
	
	memset(&nl_listener, 0, sizeof(struct crtx_netlink_listener));
	nl_listener.socket_protocol = NETLINK_ROUTE;
	nl_listener.nl_family = AF_NETLINK;
	nl_listener.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
	nl_listener.nlmsg_types = nlmsg_addr_types;
	
	nl_listener.raw2dict = &crtx_netlink_raw2dict_ifaddr;
	nl_listener.all_fields = 1;
	
	lbase = create_listener("netlink", &nl_listener);
	if (!lbase) {
		ERROR("create_listener(netlink) failed\n");
		exit(1);
	}
	
// 	crtx_create_task(lbase->graph, 0, "netlink_test", netlink_test_handler, 0);
	
	{
		ip_cache_task = create_response_cache_task("ip_cache", crtx_get_own_ip_addresses_keygen);
		ctask = (struct crtx_cache_task*) ip_cache_task->userdata;
		
		ctask->on_hit = &crtx_cache_update_on_hit;
		ctask->on_add = &crtx_cache_no_add;
		ctask->on_miss = &crtx_cache_add_on_miss;
		
		add_task(lbase->graph, ip_cache_task);
	}
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting netlink listener failed\n");
		return 1;
	}
	
	// request a list of addresses
	memset(&addr_req, 0, sizeof(addr_req));
	addr_req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	addr_req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	addr_req.n.nlmsg_type = RTM_GETADDR;
	// 	addr_req.r.ifa_family = AF_INET; // e.g., AF_INET (IPv4-only) or AF_INET6 (IPv6-only)
	
	reset_signal(&nl_listener.msg_done);
	
	crtx_netlink_send_req(&nl_listener, &addr_req.n);
	
	wait_on_signal(&nl_listener.msg_done);
	
	
	sleep(5);
	crtx_print_dict(ctask->cache->entries);
	
	free_listener(lbase);
	
	return 1;
}

struct crtx_genl_listener {
	struct nl_sock *sock;
	int id;
};

struct acpi_genl_event {
	char device_class[20];
	char bus_id[15];
	__u32 type;
	__u32 data;
};

static int my_func(struct nl_msg *msg, void *arg) {
	printf("cb\n");
	
	#define ATTR_MAX 1
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct nlattr *attrs[ATTR_MAX+1];
	
	// Validate message and parse attributes
	genlmsg_parse(nlh, 0, attrs, ATTR_MAX, 0);
	if (attrs[1]) {
		void *data;
// 		uint32_t value = nla_get_u32(attrs[ATTR_FOO]);
		data = nla_data(attrs[1]);
		printf("data %p\n", data);
		
		struct acpi_genl_event *ev = (struct acpi_genl_event *) data;
		
		printf("bus %s %s %d %d \n", ev->device_class, ev->bus_id, ev->type, ev->data);
	}
	
		
	return 0;
}

int netlink_main(int argc, char **argv) {
// 	struct crtx_netlink_listener nl_listener;
// 	struct crtx_listener_base *lbase;
// 	char ret;
	
// 	// setup a netlink listener
// 	nl_listener.socket_protocol = NETLINK_ROUTE;
// 	nl_listener.nl_family = AF_NETLINK;
// 	// e.g., RTMGRP_LINK, RTMGRP_NEIGH, RTMGRP_IPV4_IFADDR, RTMGRP_IPV6_IFADDR
// 	nl_listener.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
// 	nl_listener.nlmsg_types = nlmsg_types;
// 	
// 	// request a list of addresses
// 	memset(&addr_req, 0, sizeof(addr_req));
// 	addr_req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
// 	addr_req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
// 	addr_req.n.nlmsg_type = RTM_GETADDR;
// // 	addr_req.r.ifa_family = AF_INET; // e.g., AF_INET (IPv4-only) or AF_INET6 (IPv6-only)
// 	
// 	// request a list of interfaces
// 	memset(&if_req, 0, sizeof(if_req));
// 	if_req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
// 	if_req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
// 	if_req.n.nlmsg_type = RTM_GETLINK;
// 	if_req.ifinfomsg.ifi_family = AF_UNSPEC;
// 	
// 	crtx_get_own_ip_addresses();
	
	
	int err;
	struct crtx_genl_listener genlist;
	
	genlist.sock = nl_socket_alloc();
	if (!genlist.sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}
	
// 	nl_socket_set_buffer_size(genlist.sock, 8192, 8192);
	
	
	nl_socket_disable_seq_check(genlist.sock);
	
	
	err = nl_socket_modify_cb(genlist.sock, NL_CB_VALID, NL_CB_CUSTOM, my_func, NULL);
	printf("err %d\n", err);
	
	
	if (genl_connect(genlist.sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
// 		goto out_handle_destroy;
	}
	
// 	genlist.id = genl_ctrl_resolve(genlist.sock, "acpi_event");
// 	if (genlist.id < 0) {
// 		fprintf(stderr, "acpi_event not found.\n");
// 		err = -ENOENT;
// // 		goto out_handle_destroy;
// 	}
	

	genlist.id = genl_ctrl_resolve_grp(genlist.sock, "acpi_event", "acpi_mc_group");
// 	genlist.id = genl_ctrl_resolve_grp(genlist.sock, "nl80211", "scan");
// 	genlist.id = genl_ctrl_resolve_grp(genlist.sock, "thermal_event", "thermal_mc_grp");
	
	printf("id %d\n", genlist.id);
	if (nl_socket_add_memberships(genlist.sock, genlist.id, 0)) {
		fprintf(stderr, "Failed  nl_socket_add_memberships\n");
	}
	
	printf("start\n");
	while (1) {
		err = nl_recvmsgs_default(genlist.sock);
		if (err)
			fprintf(stderr, "err %d\n", err);
	}
	printf("end\n");
	nl_socket_free(genlist.sock);
		
	return err;
}

CRTX_TEST_MAIN(netlink_main);
#endif
