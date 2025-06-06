/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>

#include "intern.h"
#include "nl_route_raw.h"

char crtx_nl_route_send_req(struct crtx_nl_route_raw_listener *nlr_list, struct nlmsghdr *n) {
	int ret;
	
	ret = send(nlr_list->nl_listener.fd, n, n->nlmsg_len, 0);
	if (ret < 0) {
		CRTX_ERROR("error while sending netlink request: %s\n", strerror(errno));
		return 0;
	}
	
	return 1;
}

static struct crtx_dict *crtx_nl_route_raw2dict_addr(struct nlmsghdr *nlh, char all_fields) {
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
	char *family;
	
	
	ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
	rth = IFA_RTA(ifa);
	rtl = IFA_PAYLOAD(nlh);
	
	got_ifname = 0;
	
	dict = crtx_init_dict(0, 0, 0);
	
	// new or deleted address?
	{
		di = crtx_dict_alloc_next_item(dict);
		
		if (nlh->nlmsg_type == RTM_NEWADDR) {
			crtx_fill_data_item(di, 's', "type", "add", 0, CRTX_DIF_DONT_FREE_DATA);
		} else
		if (nlh->nlmsg_type == RTM_DELADDR) {
			crtx_fill_data_item(di, 's', "type", "del", 0, CRTX_DIF_DONT_FREE_DATA);
		}
		
		di = crtx_dict_alloc_next_item(dict);
		
		if (nlh->nlmsg_type == RTM_NEWADDR) {
			crtx_fill_data_item(di, 's', "action", "add", 0, CRTX_DIF_DONT_FREE_DATA);
		} else
		if (nlh->nlmsg_type == RTM_DELADDR) {
			crtx_fill_data_item(di, 's', "action", "del", 0, CRTX_DIF_DONT_FREE_DATA);
		}
	}
	
	if (all_fields) {
		di = crtx_dict_alloc_next_item(dict);
		
		switch (ifa->ifa_scope) {
			case RT_SCOPE_UNIVERSE:
				crtx_fill_data_item(di, 's', "scope", "global", 0, CRTX_DIF_DONT_FREE_DATA);
				break;
			case RT_SCOPE_SITE:
				crtx_fill_data_item(di, 's', "scope", "site", 0, CRTX_DIF_DONT_FREE_DATA);
				break;
			case RT_SCOPE_LINK:
				crtx_fill_data_item(di, 's', "scope", "link", 0, CRTX_DIF_DONT_FREE_DATA);
				break;
			case RT_SCOPE_HOST:
				crtx_fill_data_item(di, 's', "scope", "host", 0, CRTX_DIF_DONT_FREE_DATA);
				break;
			case RT_SCOPE_NOWHERE:
				crtx_fill_data_item(di, 's', "scope", "nowhere", 0, CRTX_DIF_DONT_FREE_DATA);
				break;
		}
		
		
		
		di = crtx_dict_alloc_next_item(dict);
		di->type = 'D';
		di->key = "flags";
		
		di->dict = crtx_init_dict(0, 0, 0);
		
		
		#define IFA_FLAG(name) \
		if (ifa->ifa_flags & IFA_F_ ## name) { \
			cdi = crtx_dict_alloc_next_item(di->dict); \
			crtx_fill_data_item(cdi, 's', 0, #name, 0, CRTX_DIF_DONT_FREE_DATA); \
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
			cdi->flags |= CRTX_DIF_LAST_ITEM;
	}
	
	family = 0;
	while (rtl && RTA_OK(rth, rtl)) {
		switch (rth->rta_type) {
			case IFA_LABEL:
				di = crtx_dict_alloc_next_item(dict);
				
				s = (char*) RTA_DATA(rth);
				
				slen = strlen(s);
				crtx_fill_data_item(di, 's', "interface", crtx_stracpy(s, &slen), slen, 0);
				got_ifname = 1;
				break;
				
			case IFA_ANYCAST:
			case IFA_BROADCAST:
			case IFA_MULTICAST:
			case IFA_ADDRESS:
			case IFA_LOCAL:
				di = crtx_dict_alloc_next_item(dict);
				
				memset(&addr, 0, sizeof(addr));
				addr.ss_family = ifa->ifa_family;
				if (addr.ss_family == AF_INET) {
					
					struct sockaddr_in *sin;
					
					sin = (struct sockaddr_in *) &addr;
					memcpy(&sin->sin_addr, RTA_DATA(rth), sizeof(sin->sin_addr));
					
					s = inet_ntop(ifa->ifa_family, &(((struct sockaddr_in *)&addr)->sin_addr), s_addr, sizeof(s_addr));
					if (!s) {
						CRTX_ERROR("inet_ntop(v4) failed: %s\n", strerror(errno));
					}
					
					if (!family) {
						family = "ipv4";
						crtx_fill_data_item(di, 's', "family", family, 0, CRTX_DIF_DONT_FREE_DATA);
						di = crtx_dict_alloc_next_item(dict);
					}
				} else
				if (addr.ss_family == AF_INET6) {
					struct sockaddr_in6 *sin6;
					
					sin6 = (struct sockaddr_in6 *) &addr;
					memcpy(&sin6->sin6_addr, RTA_DATA(rth), sizeof(sin6->sin6_addr));
					
					s = inet_ntop(ifa->ifa_family, &(((struct sockaddr_in6 *)&addr)->sin6_addr), s_addr, sizeof(s_addr));
					if (!s) {
						CRTX_ERROR("inet_ntop(v6) failed: %s\n", strerror(errno));
					}
					
					if (!family) {
						family = "ipv6";
						crtx_fill_data_item(di, 's', "family", family, 0, CRTX_DIF_DONT_FREE_DATA);
						di = crtx_dict_alloc_next_item(dict);
					}
				} else {
					CRTX_ERROR("unknown family: %d\n", addr.ss_family);
					s = 0;
				}
				
				if (s) {
					char *key;
					
					key = 0;
					switch (rth->rta_type) {
						case IFA_ANYCAST: key = "anycast"; break;
						case IFA_BROADCAST: key = "broadcast"; break;
						case IFA_MULTICAST: key = "multicast"; break;
						case IFA_ADDRESS:
							key = "address";
							crtx_fill_data_item(di, 'u', "prefixlen", ifa->ifa_prefixlen, sizeof(ifa->ifa_prefixlen), 0);
							di = crtx_dict_alloc_next_item(dict);
							break;
						case IFA_LOCAL: key = "local"; break;
					}
					
					if (key) {
						slen = strlen(s);
						crtx_fill_data_item(di, 's', key, crtx_stracpy(s, &slen), slen, 0);
					}
				}
				
				break;
			
			case IFA_CACHEINFO:
				if (all_fields) {
					di = crtx_dict_alloc_next_item(dict);
					di->type = 'D';
					di->key = "cache";
					
					di->dict = crtx_init_dict("uuuu", 4, 0);
					
					cinfo = (struct ifa_cacheinfo*) RTA_DATA(rth);
					
					cdi = crtx_get_first_item(di->dict);
					crtx_fill_data_item(cdi, 'u', "valid_lft", cinfo->ifa_valid, 0, 0);
					cdi = crtx_get_next_item(di->dict, cdi);
					crtx_fill_data_item(cdi, 'u', "preferred_lft", cinfo->ifa_prefered, 0, 0);
					cdi = crtx_get_next_item(di->dict, cdi);
					crtx_fill_data_item(cdi, 'u', "cstamp", cinfo->cstamp, 0, 0);
					cdi = crtx_get_next_item(di->dict, cdi);
					crtx_fill_data_item(cdi, 'u', "tstamp", cinfo->tstamp, 0, 0);
					
					cdi->flags |= CRTX_DIF_LAST_ITEM;
				}
				
				// 0xFFFFFFFFUL == forever
				
				break;
				
			case IFA_FLAGS:
				// we get this from ifa->ifa_flags
				
// 				if (nlr_list->all_fields) {
// 					unsigned int *flags;
// 					
// 					di = crtx_dict_alloc_next_item(dict);
// 					di->type = 'D';
// 					di->key = "flags";
// 					
// 					di->dict = crtx_init_dict(0, 0, 0);
// 					
// 					flags = (unsigned int*) RTA_DATA(rth);
// 					
// 					#define IFA_FLAG(name)
// 						if (*flags & IFA_F_ ## name) {
// 							cdi = crtx_dict_alloc_next_item(di->dict);
// 							crtx_fill_data_item(cdi, 's', 0, #name, 0, CRTX_DIF_DONT_FREE_DATA);
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
// 						cdi->flags |= CRTX_DIF_LAST_ITEM;
// 				}
				break;
				
			default:
					CRTX_VDBG("nl_route: unhandled rta_type %d\n", rth->rta_type);
		}
		rth = RTA_NEXT(rth, rtl);
	}
	
	di = crtx_dict_alloc_next_item(dict);
	crtx_fill_data_item(di, 'i', "interface_index", ifa->ifa_index, sizeof(ifa->ifa_index), 0);
	
	if (!got_ifname) {
		di = crtx_dict_alloc_next_item(dict);
		
		if_indextoname(ifa->ifa_index, ifname);
		slen = strlen(ifname);
		crtx_fill_data_item(di, 's', "interface", crtx_stracpy(ifname, &slen), slen, 0);
	}
	
	di->flags |= CRTX_DIF_LAST_ITEM;
	
	return dict;
}

struct crtx_dict *crtx_nl_route_raw2dict_interface(struct nlmsghdr *nlh, char all_fields) {
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
		di = crtx_dict_alloc_next_item(dict);
		
		if (nlh->nlmsg_type == RTM_NEWLINK) {
			crtx_fill_data_item(di, 's', "type", "add", 0, CRTX_DIF_DONT_FREE_DATA);
		} else
		if (nlh->nlmsg_type == RTM_DELLINK) {
			crtx_fill_data_item(di, 's', "type", "del", 0, CRTX_DIF_DONT_FREE_DATA);
		}
		
		di = crtx_dict_alloc_next_item(dict);
		
		if (nlh->nlmsg_type == RTM_NEWLINK) {
			crtx_fill_data_item(di, 's', "action", "add", 0, CRTX_DIF_DONT_FREE_DATA);
		} else
		if (nlh->nlmsg_type == RTM_DELLINK) {
			crtx_fill_data_item(di, 's', "action", "del", 0, CRTX_DIF_DONT_FREE_DATA);
		}
	}
	
	{
		di = crtx_dict_alloc_next_item(dict);
		di->type = 'i';
		di->key = "index";
		
		crtx_fill_data_item(di, 'i', "index", ifi->ifi_index, sizeof(ifi->ifi_index), 0);
	}
	
	{
		di = crtx_dict_alloc_next_item(dict);
		di->type = 'D';
		di->key = "flags";
		di->dict = crtx_init_dict(0, 0, 0);
		
		#define IFI_FLAG(name) \
		if (ifi->ifi_flags & IFF_ ## name) { \
			cdi = crtx_dict_alloc_next_item(di->dict); \
			crtx_fill_data_item(cdi, 's', #name, #name, 0, CRTX_DIF_DONT_FREE_DATA); \
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
				di = crtx_dict_alloc_next_item(dict);
				
				// e.g., sit0 interfaces use only 4 bytes
				if (RTA_PAYLOAD(rth) > 6)
					CRTX_INFO("netlink_raw: RTA_PAYLOAD %zu > 6\n", RTA_PAYLOAD(rth));
				
				s = (char*) malloc(RTA_PAYLOAD(rth)*3);
				b = (unsigned char*) RTA_DATA(rth);
				
				snprintf(s, RTA_PAYLOAD(rth)*3, "%02x:%02x:%02x:%02x:%02x:%02x", 
						 b[0], b[1], b[2], b[3], b[4], b[5]);
				
				key = 0;
				switch (rth->rta_type) {
					case IFLA_ADDRESS: key = "address"; break;
					case IFLA_BROADCAST: key = "broadcast"; break;
				}
				if (key)
					crtx_fill_data_item(di, 's', key, s, RTA_PAYLOAD(rth)*3-1, 0);
				
				break;
				
			case IFLA_IFNAME:
				di = crtx_dict_alloc_next_item(dict);
				
				s = (char*) RTA_DATA(rth);
				
				slen = strlen(s);
				crtx_fill_data_item(di, 's', "interface", crtx_stracpy(s, &slen), slen, 0);
				break;
				
			case IFLA_MTU:
				di = crtx_dict_alloc_next_item(dict);
				crtx_fill_data_item(di, 'u', "mtu", *((unsigned int*) RTA_DATA(rth)), 0, 0);
				break;
				
			case IFLA_LINK:
				di = crtx_dict_alloc_next_item(dict);
				crtx_fill_data_item(di, 'i', "link", *((int*) RTA_DATA(rth)), 0, 0);
				break;
				
			case IFLA_QDISC:
				s = (char*) RTA_DATA(rth);
				
				slen = strlen(s);
				
				di = crtx_dict_alloc_next_item(dict);
				crtx_fill_data_item(di, 's', "qdisc", crtx_stracpy(s, &slen), slen, 0);
				break;
				
			case IFLA_STATS:
				if (all_fields) {
					di = crtx_dict_alloc_next_item(dict);
					di->type = 'D';
					di->key = "stats";
					di->dict = crtx_init_dict(0, 23, 0);
					
					stats = (struct rtnl_link_stats *) RTA_DATA(rth);
					
					#define ADD_ITEM(attr) cdi = crtx_get_next_item(di->dict, cdi); \
					crtx_fill_data_item(cdi, 'u', #attr, stats->attr, 0, 0);
					
					cdi = crtx_get_first_item(di->dict);
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
				CRTX_VDBG("nl_route: unhandled rta_type %d\n", rth->rta_type);
		}
		
		rth = RTA_NEXT(rth, len);
	}
	
	di->flags |= CRTX_DIF_LAST_ITEM;
	
	return dict;
}

struct crtx_dict * crtx_nl_route_raw2dict_neigh(struct nlmsghdr *nlh, char all_fields) {
	struct crtx_dict *dict;
	struct crtx_dict_item *di;
	struct ndmsg *ndmsg;
	struct rtattr *rth;
	int32_t len;

# define NDA_RTA(r) \
	((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
	
	ndmsg = NLMSG_DATA(nlh);
	rth = NDA_RTA(ndmsg);
	len = NLMSG_PAYLOAD(nlh, sizeof(struct ndmsg));
	
	dict = crtx_init_dict(0, 0, 0);
	
	crtx_dict_new_item(dict, 'u', "ndm_family", ndmsg->ndm_family, sizeof(ndmsg->ndm_family), 0);
	crtx_dict_new_item(dict, 'u', "ndm_pad1", ndmsg->ndm_pad1, sizeof(ndmsg->ndm_pad1), 0);
	crtx_dict_new_item(dict, 'u', "ndm_pad2", ndmsg->ndm_pad2, sizeof(ndmsg->ndm_pad2), 0);
	crtx_dict_new_item(dict, 'i', "ndm_ifindex", ndmsg->ndm_ifindex, sizeof(ndmsg->ndm_ifindex), 0);
// 	crtx_dict_new_item(dict, 'u', "ndm_state", ndmsg->ndm_state, sizeof(ndmsg->ndm_state), 0);
	crtx_dict_new_item(dict, 'u', "ndm_flags", ndmsg->ndm_flags, sizeof(ndmsg->ndm_flags), 0);
	crtx_dict_new_item(dict, 'u', "ndm_type", ndmsg->ndm_type, sizeof(ndmsg->ndm_type), 0);
	
	di = crtx_dict_alloc_next_item(dict);
	di->type = 'D';
	di->key = "ndm_state";
	di->dict = crtx_init_dict(0, 0, 0);
	
#define CRTX_NDM_STATE(item) \
	if (ndmsg->ndm_state & NUD_ ## item) { \
		crtx_dict_new_item(di->dict, 's', "", #item, 0, CRTX_DIF_DONT_FREE_DATA); \
	}
	CRTX_NDM_STATE(INCOMPLETE);
	CRTX_NDM_STATE(REACHABLE);
	CRTX_NDM_STATE(STALE);
	CRTX_NDM_STATE(DELAY);
	CRTX_NDM_STATE(PROBE);
	CRTX_NDM_STATE(FAILED);
	CRTX_NDM_STATE(NOARP);
	CRTX_NDM_STATE(PERMANENT);
	
	
	
	di = crtx_dict_alloc_next_item(dict);
	di->type = 'D';
	di->key = "ndm_flags";
	di->dict = crtx_init_dict(0, 0, 0);
	
#define CRTX_NDM_FLAGS(item) \
	if (ndmsg->ndm_flags & NTF_ ## item) { \
		crtx_dict_new_item(di->dict, 's', "", #item, 0, CRTX_DIF_DONT_FREE_DATA); \
	}
	
	CRTX_NDM_FLAGS(PROXY);
	CRTX_NDM_FLAGS(ROUTER);
	
	while (len && RTA_OK(rth, len)) {
// 		switch (rth->rta_type) {
// 			NDA_UNSPEC,
// 			NDA_DST,
// 			NDA_LLADDR,
// 			NDA_CACHEINFO,
// 			NDA_PROBES,
// 			NDA_VLAN,
// 			NDA_PORT,
// 			NDA_VNI,
// 			NDA_IFINDEX,
// 			NDA_MASTER,
// 			NDA_LINK_NETNSID,
// 		}
		rth = RTA_NEXT(rth, len);
	}
	
	return dict;
}

struct crtx_dict *crtx_nl_route_raw2dict_ifaddr(struct crtx_nl_route_raw_listener *nlr_list, struct nlmsghdr *nlh) {
	switch (nlh->nlmsg_type) {
		case RTM_NEWADDR:
		case RTM_DELADDR:
			return crtx_nl_route_raw2dict_addr(nlh, nlr_list->all_fields);
			break;
		case RTM_NEWLINK:
		case RTM_DELLINK:
			return crtx_nl_route_raw2dict_interface(nlh, nlr_list->all_fields);
			break;
	}
	
	return 0;
}

struct crtx_dict *crtx_nl_route_raw2dict_ifaddr2(struct nlmsghdr *nlh, int all_fields) {
	switch (nlh->nlmsg_type) {
		case RTM_NEWADDR:
		case RTM_DELADDR:
			return crtx_nl_route_raw2dict_addr(nlh, all_fields);
			break;
		case RTM_NEWLINK:
		case RTM_DELLINK:
			return crtx_nl_route_raw2dict_interface(nlh, all_fields);
			break;
		case RTM_NEWNEIGH:
		case RTM_DELNEIGH:
			return crtx_nl_route_raw2dict_neigh(nlh, all_fields);
			break;
	}
	
	return 0;
}

static int nl_route_read_cb(struct crtx_netlink_raw_listener *nl_listener, int fd, void *userdata) {
	int len;
	char buffer[4096];
	struct nlmsghdr *nlh;
	struct crtx_event *event;
	struct crtx_nl_route_raw_listener *nlr_list;
	
	nlh = (struct nlmsghdr *) buffer;
	
	nlr_list = (struct crtx_nl_route_raw_listener*) userdata;
	
	len = recv(fd, nlh, 4096, 0);
	if (len > 0) {
		while ((NLMSG_OK(nlh, len)) && (nlh->nlmsg_type != NLMSG_DONE)) {
			uint16_t *it;
			
			it = nlr_list->nlmsg_types;
			while (*it && *it != nlh->nlmsg_type) { it++;}
			
			if (nlh->nlmsg_type == *it) {
				char *ptr;
				crtx_create_event(&event);
				
				if (!nlr_list->raw2dict) {
					// we don't know the size of data, hence we cannot copy it
					// and we have to wait until all tasks have finished
					
					ptr = malloc(4096);
					memcpy(ptr, nlh, 4096);
					crtx_event_set_raw_data(event, 'p', ptr, sizeof(ptr), 0);
				} else {
					crtx_event_set_dict_data(event, nlr_list->raw2dict(nlr_list, nlh), 0);
					
				}
				crtx_add_event(nlr_list->base.graph, event);
			}
			
			nlh = NLMSG_NEXT(nlh, len);
		}
		
		if (nlh->nlmsg_type == NLMSG_DONE) {
			if (nlr_list->msg_done_cb) {
				nlr_list->msg_done_cb(nlr_list->msg_done_cb_data);
			} else {
				crtx_create_event(&event);
				event->description = "done";
				crtx_add_event(nlr_list->base.graph, event);
			}
		}
	}
	
	if (len < 0) {
		CRTX_ERROR("netlink_raw recv failed: %s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}

static char nl_route_start_listener(struct crtx_listener_base *listener) {
	struct crtx_nl_route_raw_listener *nlr_list;
	int ret;
	
	nlr_list = (struct crtx_nl_route_raw_listener*) listener;
	
	ret = crtx_start_listener(&nlr_list->nl_listener.base);
	if (ret) {
		CRTX_ERROR("starting netlink_raw listener failed\n");
		return ret;
	}
	
	return 0;
}

static char nl_route_stop_listener(struct crtx_listener_base *listener) {
	struct crtx_nl_route_raw_listener *nlr_list;
	
	nlr_list = (struct crtx_nl_route_raw_listener*) listener;
	
	crtx_stop_listener(&nlr_list->nl_listener.base);
	
	return 0;
}

static void shutdown_nl_route_listener(struct crtx_listener_base *lbase) {
	struct crtx_nl_route_raw_listener *nlr_list;
	
	nlr_list = (struct crtx_nl_route_raw_listener*) lbase;
	
	crtx_shutdown_listener(&nlr_list->nl_listener.base);
	nlr_list->base.graph = 0;
}


struct crtx_listener_base *crtx_setup_nl_route_raw_listener(void *options) {
	struct crtx_nl_route_raw_listener *nlr_list;
	int ret;
	
	nlr_list = (struct crtx_nl_route_raw_listener*) options;
	
	nlr_list->base.mode = CRTX_NO_PROCESSING_MODE;
	
	nlr_list->nl_listener.socket_protocol = NETLINK_ROUTE;
	nlr_list->nl_listener.nl_family = AF_NETLINK;
// 	nlr_list->nl_listener.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
// 	nlr_list->nl_listener.nlmsg_types = nlmsg_addr_types;
	nlr_list->nl_listener.read_cb = nl_route_read_cb;
	nlr_list->nl_listener.read_cb_userdata = nlr_list;
	
	nlr_list->raw2dict = &crtx_nl_route_raw2dict_ifaddr;
	nlr_list->all_fields = 1;
	
	ret = crtx_setup_listener("netlink_raw", &nlr_list->nl_listener);
	if (ret) {
		CRTX_ERROR("create_listener(netlink_raw) failed: %s\n", strerror(-ret));
		return 0;
	}
	
	nlr_list->base.graph = nlr_list->nl_listener.base.graph;
	
	nlr_list->base.shutdown = &shutdown_nl_route_listener;
	nlr_list->base.start_listener = &nl_route_start_listener;
	nlr_list->base.stop_listener = &nl_route_stop_listener;
	
	return &nlr_list->base;
}

void crtx_nl_route_raw_clear_listener(struct crtx_nl_route_raw_listener *lstnr) {
	memset(lstnr, 0, sizeof(struct crtx_nl_route_raw_listener));
}

CRTX_DEFINE_ALLOC_FUNCTION(nl_route_raw)
CRTX_DEFINE_CALLOC_FUNCTION(nl_route_raw)

void crtx_nl_route_raw_init() {
}

void crtx_nl_route_raw_finish() {
}

