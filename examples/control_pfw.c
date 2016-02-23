
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <ctype.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


#include "core.h"
#include "cache.h"
#include "nf_queue.h"
#include "controls.h"
#include "dict.h"

struct crtx_nfq_listener nfq_list;
struct crtx_listener_base *nfq_list_base;
struct crtx_graph *rl_graph, *newp_graph; //, *newp_raw_graph;
struct crtx_task *rcache_host, *rcache_ip;


// iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW  -m mark --mark 0 -j NFQUEUE --queue-num 0
// iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW -m mark --mark 1 -j REJECT
// iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW -m mark --mark 2 -j ACCEPT
// iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW -m mark --mark 3 -j

#define PFW_DEFAULT 3
#define PFW_ACCEPT 2
#define PFW_REJECT 1

#ifndef PFW_DATA_DIR
#define PFW_DATA_DIR "/etc/cortexd/pfw/"
#endif

#define PFW_NEWPACKET_ETYPE "pfw/newpacket"
char *newp_event_types[] = { PFW_NEWPACKET_ETYPE, 0 };

#define PFW_NEWPACKET_RAW_ETYPE "pfw/newpacket_raw"
char *newp_raw_event_types[] = { PFW_NEWPACKET_RAW_ETYPE, 0 };


struct ip_ll {
	struct ip_ll *next;
	
	char ip[0];
};

struct ip_ll *local_ips = 0;


struct filter_set {
	char **list;
	regex_t *rlist;
	
	size_t length;
};

struct filter_set ip_whitelist;
struct filter_set ip_blacklist;
struct filter_set host_whitelist;
struct filter_set host_blacklist;

struct filter_set host_nocachelist;
struct filter_set ip_nocachelist;

struct filter_set resolvelist;

void print_filter_set(struct filter_set *fset, char *name) {
	size_t i;
	
	printf("%s:\n", name);
	
	for (i=0; i<fset->length; i++) {
		printf("  %s\n", fset->list[i]);
	}
	printf("\n");
}

void add_ips_to_list(struct filter_set *fset, char *hostname) {
	struct addrinfo* result;
	struct addrinfo* res;
	int error, ret;
	size_t i;
	
	error = getaddrinfo(hostname, NULL, NULL, &result);
	if (error != 0) {
		if (error == EAI_SYSTEM) {
			perror("getaddrinfo");
		} else {
			fprintf(stderr, "error in getaddrinfo(%s): %s\n", hostname, gai_strerror(error));
		}
		return;
	}
	
	for (res = result; res != NULL; res = res->ai_next) {
		if (res->ai_family != AF_INET)
			continue;
		
		struct sockaddr_in* saddr = (struct sockaddr_in*)res->ai_addr;
// 		printf("hostname: %s\n", inet_ntoa(saddr->sin_addr));
		
		char cont = 0;
		for (i=0; i<fset->length; i++) {
			if (!strcmp(fset->list[i], inet_ntoa(saddr->sin_addr))) {
				cont = 1;
				break;
			}
		}
		if (cont)
			continue;
		printf("add %s %s\n", inet_ntoa(saddr->sin_addr), hostname);
		fset->length++;
		fset->list = (char**) realloc(fset->list, sizeof(char*)*fset->length);
		fset->rlist = (regex_t*) realloc(fset->rlist, sizeof(regex_t)*fset->length);
		fset->list[fset->length-1] = stracpy(inet_ntoa(saddr->sin_addr), 0);
		
		ret = regcomp(&fset->rlist[fset->length-1], fset->list[fset->length-1], 0);
		if (ret) {
			fprintf(stderr, "Could not compile regex %s\n", fset->list[fset->length-1]);
// 			exit(1);
		}
	}
	
	freeaddrinfo(result);
	return;
}

// static void pfw_main_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct crtx_event *newp_raw_event, *newp_event;
// 	struct crtx_nfq_packet *ev;
// 	
// 	if (event->data.raw.size < sizeof(struct crtx_nfq_packet))
// 		return;
// 	
// 	ev = (struct crtx_nfq_packet*) event->data.raw.pointer;
// 	
// 	// if someone set a response already, ignore
// // 	if (event->response.raw.pointer && event->response.raw.pointer != &ev->mark_out)
// // 		return;
// 	
// 	ev->mark_out = PFW_DEFAULT;
// 	
// // 	if (newp_raw_graph && newp_raw_graph->tasks) {
// // 		newp_raw_event = create_event(PFW_NEWPACKET_ETYPE, event->data.raw.pointer, event->data.raw.size);
// // 		
// // 		add_event(newp_graph, newp_raw_event);
// // 		
// // 		wait_on_event(newp_raw_event);
// // 		
// // 		free_event(newp_raw_event);
// // 	}
// 	
// 	if (newp_graph && newp_graph->tasks) {
// // 		int res;
// // 		char host[256];
// // 		struct iphdr *iph;
// // 		size_t slen;
// // // 		char *sign;
// // 		struct crtx_dict_item *di;
// // 		struct crtx_dict *ds;
// // 		struct sockaddr_in sa;
// // 		
// // 		iph = (struct iphdr*)ev->payload;
// // 		sa.sin_family = AF_INET;
// // 		
// // 		ds = crtx_init_dict(PFW_NEWPACKET_SIGNATURE, strlen(PFW_NEWPACKET_SIGNATURE), 0);
// // 		di = ds->items;
// // 		
// // 		crtx_fill_data_item(di, 'u', "protocol", iph->protocol, 0, 0); di++;
// // 		
// // 		{
// // 			slen = 0;
// // 			crtx_fill_data_item(di, 's', "src_ip", stracpy(inet_ntoa(*(struct in_addr *)&iph->saddr), &slen), slen, 0); di++;
// // 			
// // 			sa.sin_addr = *(struct in_addr *)&iph->saddr;
// // 			res = getnameinfo((struct sockaddr*) &sa, sizeof(struct sockaddr), host, 64, 0, 0, 0);
// // 			if (res) {
// // 				printf("getnameinfo failed %d\n", res);
// // 				host[0] = 0;
// // 			}
// // 			
// // 			slen = strlen(host);
// // 			crtx_fill_data_item(di, 's', "src_hostname", stracpy(host, &slen), slen, 0); di++;
// // 		}
// // 		
// // 		{
// // 			slen = 0;
// // 			crtx_fill_data_item(di, 's', "dst_ip", stracpy(inet_ntoa(*(struct in_addr *)&iph->daddr), &slen), slen, 0); di++;
// // 			
// // 			sa.sin_addr = *(struct in_addr *)&iph->daddr;
// // 			res = getnameinfo((struct sockaddr*) &sa, sizeof(struct sockaddr), host, 64, 0, 0, 0);
// // 			if (res) {
// // 				printf("getnameinfo failed %d\n", res);
// // 				host[0] = 0;
// // 			}
// // 			
// // 			slen = strlen(host);
// // 			crtx_fill_data_item(di, 's', "dst_hostname", stracpy(host, &slen), slen, 0); di++;
// // 		}
// // 		
// // 		crtx_fill_data_item(di, 'D', "payload", 0, 0, DIF_LAST);
// // 		
// // 		if (iph->protocol == 6) {
// // 			struct crtx_dict_item *pdi;
// // 			struct tcphdr *tcp;
// // 			
// // 			tcp = (struct tcphdr *) (ev->payload + (iph->ihl << 2));
// // 			
// // 			di->ds = crtx_init_dict(PFW_NEWPACKET_TCP_SIGNATURE, strlen(PFW_NEWPACKET_TCP_SIGNATURE), 0);
// // 			pdi = di->ds->items;
// // 			
// // 			crtx_fill_data_item(pdi, 'u', "src_port", ntohs(tcp->source), 0, 0); pdi++;
// // 			
// // 			crtx_fill_data_item(pdi, 'u', "dst_port", ntohs(tcp->dest), 0, DIF_LAST);
// // 			
// // // 			fprintf(stdout, "TCP{sport=%u; dport=%u; seq=%u; ack_seq=%u; flags=u%ua%up%ur%us%uf%u; window=%u; urg=%u}\n",
// // // 				   ntohs(tcp->source), ntohs(tcp->dest), ntohl(tcp->seq), ntohl(tcp->ack_seq)
// // // 				   ,tcp->urg, tcp->ack, tcp->psh, tcp->rst, tcp->syn, tcp->fin, ntohs(tcp->window), tcp->urg_ptr);
// // 		} else
// // 		if (iph->protocol == 17) {
// // 			struct crtx_dict_item *pdi;
// // 			struct udphdr *udp;
// // 			
// // 			udp = (struct udphdr *) (ev->payload + (iph->ihl << 2));
// // 			
// // 			di->ds = crtx_init_dict(PFW_NEWPACKET_UDP_SIGNATURE, strlen(PFW_NEWPACKET_UDP_SIGNATURE), 0);
// // 			pdi = di->ds->items;
// // 			
// // 			crtx_fill_data_item(pdi, 'u', "src_port", ntohs(udp->source), 0, 0); pdi++;
// // 			
// // 			crtx_fill_data_item(pdi, 'u', "dst_port", ntohs(udp->dest), 0, DIF_LAST);
// // 			
// // // 			fprintf(stdout,"UDP{sport=%u; dport=%u; len=%u}\n",
// // // 				   ntohs(udp->source), ntohs(udp->dest), udp->len);
// // 		}
// 		
// 		newp_event = create_event(PFW_NEWPACKET_ETYPE, 0, 0);
// 		newp_event->data.dict = ds;
// 		newp_event->data.raw.pointer = event->data.raw.pointer;
// 		newp_event->data.raw.size = event->data.raw.size;
// 		newp_event->response.copy_raw = &crtx_copy_raw_data;
// 		
// 		reference_event_release(newp_event);
// 		
// 		add_event(newp_graph, newp_event);
// 		
// 		wait_on_event(newp_event);
// 		
// 		event->response.raw.uint32 = newp_event->response.raw.uint32;
// 		event->response.raw.type = 'u';
// 		
// 		// do not free as referenced data does not belong to us
// 		newp_event->data.raw.pointer = 0;
// 		newp_event->response.raw.pointer = 0;
// 		dereference_event_release(newp_event);
// 	}
// }

char pfw_is_ip_local(char *ip) {
	struct ip_ll *iit;
	
	for (iit=local_ips; iit; iit=iit->next) {
		if (!strcmp(iit->ip, ip))
			return 1;
	}
	
	return 0;
}

void pfw_get_remote_part(struct crtx_dict *ds, char **remote_ip, char **remote_host) {
	char src_local, dst_local;
	unsigned int i_remote_ip, i_remote_host;
	
	if (strcmp(ds->signature, PFW_NEWPACKET_SIGNATURE)) {
		printf("error, signature mismatch: %s %s\n", ds->signature, PFW_NEWPACKET_SIGNATURE);
		return;
	}
	
	if (pfw_is_ip_local(ds->items[PFW_NEWP_SRC_IP].string)) {
		src_local = 1;
	} else {
		src_local = 0;
		if (pfw_is_ip_local(ds->items[PFW_NEWP_DST_IP].string))
			dst_local = 1;
		else
			dst_local = 0;
	}
	
	if (!src_local && !dst_local) {
		i_remote_ip = PFW_NEWP_SRC_IP;
		i_remote_host = PFW_NEWP_SRC_HOST;
	} else {
		if (src_local) {
			i_remote_ip = PFW_NEWP_DST_IP;
			i_remote_host = PFW_NEWP_DST_HOST;
		} else {
			i_remote_ip = PFW_NEWP_SRC_IP;
			i_remote_host = PFW_NEWP_SRC_HOST;
		}
	}
	
	*remote_host = ds->items[i_remote_host].string;
	*remote_ip = ds->items[i_remote_ip].string;
}

char pfw_rcache_create_key_ip(struct crtx_event *event, struct crtx_dict_item *key) {
	struct crtx_dict *ds;
	char *remote_ip, *remote_host;
	
	if (!event->data.dict)
		event->data.raw_to_dict(&event->data);
	
	ds = event->data.dict;
	
	pfw_get_remote_part(ds, &remote_ip, &remote_host);
	
	key->type = 's';
	key->string = stracpy(remote_ip, 0);
	if (!key->string)
		return 0;
	key->flags |= DIF_KEY_ALLOCATED;
	
	return 1;
// 	return stracpy(remote_ip, 0);
}

char pfw_rcache_create_key_host(struct crtx_event *event, struct crtx_dict_item *key) {
	struct crtx_dict *ds;
	char *remote_ip, *remote_host;
	
	if (!event->data.dict)
		event->data.raw_to_dict(&event->data);
	
	ds = event->data.dict;
	
	pfw_get_remote_part(ds, &remote_ip, &remote_host);
	
	key->type = 's';
	key->string = stracpy(remote_host, 0);
	if (!key->string)
		return 0;
	key->flags |= DIF_KEY_ALLOCATED;
	
	return 1;
// 	return stracpy(remote_host, 0);
}

static void pfw_print_packet(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *ds, *pds;
	char src_local, dst_local;
	
	ds = event->data.dict;
	
	if (strcmp(ds->signature, PFW_NEWPACKET_SIGNATURE)) {
		printf("error, signature mismatch: %s %s\n", ds->signature, PFW_NEWPACKET_SIGNATURE);
		return;
	}
	
	if (pfw_is_ip_local(ds->items[PFW_NEWP_SRC_IP].string)) {
		src_local = 1;
	} else {
		src_local = 0;
		if (pfw_is_ip_local(ds->items[PFW_NEWP_DST_IP].string))
			dst_local = 1;
		else
			dst_local = 0;
	}
	
	printf("%s ", crtx_nfq_proto2str(ds->items[PFW_NEWP_PROTOCOL].uint32));
	
	if (!src_local && !dst_local) {
		printf("brd %s %s ", ds->items[PFW_NEWP_SRC_IP].string, ds->items[PFW_NEWP_SRC_HOST].string);
	} else {
		if (src_local) {
			printf("dst %s %s ", ds->items[PFW_NEWP_DST_IP].string, ds->items[PFW_NEWP_DST_HOST].string);
		} else {
			printf("src %s %s ", ds->items[PFW_NEWP_SRC_IP].string, ds->items[PFW_NEWP_SRC_HOST].string);
		}
	}
	
	if (ds->items[PFW_NEWP_PROTOCOL].uint32 == 6) {
		pds = ds->items[PFW_NEWP_PAYLOAD].ds;
		
		if (strcmp(pds->signature, PFW_NEWPACKET_TCP_SIGNATURE)) {
			printf("error, signature mismatch: %s %s\n", pds->signature, PFW_NEWPACKET_TCP_SIGNATURE);
			return;
		}
		
		printf("%u %u ", pds->items[PFW_NEWP_TCP_SPORT].uint32, pds->items[PFW_NEWP_TCP_DPORT].uint32);
	} else
	if (ds->items[PFW_NEWP_PROTOCOL].uint32 == 17) {
		pds = ds->items[PFW_NEWP_PAYLOAD].ds;
		
		if (strcmp(pds->signature, PFW_NEWPACKET_UDP_SIGNATURE)) {
			printf("error, signature mismatch: %s %s\n", pds->signature, PFW_NEWPACKET_UDP_SIGNATURE);
			return;
		}
		
		printf("%u %u ", pds->items[PFW_NEWP_UDP_SPORT].uint32, pds->items[PFW_NEWP_UDP_DPORT].uint32);
	}
	printf("\n");
}

void free_list(struct filter_set *fset) {
	size_t i;
	
	for (i=0; i<fset->length; i++) {
		free(fset->list[i]);
		regfree(&fset->rlist[i]);
	}
	free(fset->list);
	free(fset->rlist);
}

void load_list(struct filter_set *fset, char *file) {
	FILE *f;
	int ret;
	char buf[1024];
	char *c;
	
	f = fopen(file, "r");
	
	if (!f)
		return;
	
	while (fgets(buf, sizeof(buf), f)) {
		c=buf;
		while (*c && isspace(*c))
			c++;
		
		if (! *c)
			continue;
		
		fset->length++;
		fset->list = (char**) realloc(fset->list, sizeof(char*)*fset->length);
		fset->rlist = (regex_t*) realloc(fset->rlist, sizeof(regex_t)*fset->length);
		size_t slen = strlen(buf)-1; // don't copy newline
		fset->list[fset->length-1] = stracpy(buf, &slen);
		
		ret = regcomp(&fset->rlist[fset->length-1], fset->list[fset->length-1], 0);
		if (ret) {
			fprintf(stderr, "Could not compile regex %s\n", fset->list[fset->length-1]);
			exit(1);
		}
	}
	
	if (ferror(stdin)) {
		fprintf(stderr,"Oops, error reading stdin\n");
		exit(1);
	}
	
	fclose(f);
}

static char match_regexp_list(char *input, struct filter_set *fset) {
	int i, ret;
	char msgbuf[128];
	
	for (i=0; i<fset->length; i++) {
// 		printf("match %s %s\n", input, fset->list[i]);
		ret = regexec(&fset->rlist[i], input, 0, NULL, 0);
		if (!ret) {
			return 1;
		} else if (ret == REG_NOMATCH) {
			
		} else {
			regerror(ret, &fset->rlist[i], msgbuf, sizeof(msgbuf));
			fprintf(stderr, "Regex match failed: %s\n", msgbuf);
			exit(1);
		}
	}
	return 0;
}

static void pfw_rules_filter(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *ds;
	char *remote_ip, *remote_host;
	
	if (event->response.raw.pointer)
		return;
	
	if (!event->data.dict)
		event->data.raw_to_dict(&event->data);
	
	ds = event->data.dict;
	
	pfw_get_remote_part(ds, &remote_ip, &remote_host);
	
/* 	#define SET_MARK(event, mark) { \
// 			(event)->response.raw.pointer = &((struct crtx_nfq_packet*) (event)->data.raw.pointer)->mark_out; \
// 			(event)->response.raw.size = sizeof(u_int32_t); \
// 			((struct crtx_nfq_packet*) (event)->data.raw.pointer)->mark_out = (mark); \
//  		}
*/
	
	#define SET_MARK(event, mark) { (event)->response.raw.type = 'u'; (event)->response.raw.uint32 = (mark); }
	if (match_regexp_list(remote_host, &host_blacklist)) {
		SET_MARK(event, PFW_REJECT);
		return;
	}
	
	if (match_regexp_list(remote_ip, &ip_blacklist)) {
		SET_MARK(event, PFW_REJECT);
		return;
	}
	
	if (match_regexp_list(remote_host, &host_whitelist)) {
		SET_MARK(event, PFW_ACCEPT);
		return;
	}
	
	if (match_regexp_list(remote_ip, &ip_whitelist)) {
		SET_MARK(event, PFW_ACCEPT);
		return;
	}
	
	SET_MARK(event, PFW_DEFAULT);
}

char host_cache_add_cb(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event) {
	if (match_regexp_list(key->string, &host_nocachelist))
		return 0;
	else
		return 1;
}

char ip_cache_add_cb(struct crtx_cache_task *ct, struct crtx_dict_item *key, struct crtx_event *event) {
	if (match_regexp_list(key->string, &ip_nocachelist))
		return 0;
	else
		return 1;
}

char init() {
	struct ifaddrs *addrs, *tmp;
	struct ip_ll *iit;
	char *s;
	
	getifaddrs(&addrs);
	tmp = addrs;
	
	while (tmp) {
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
		{
			struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
			
			s = inet_ntoa(pAddr->sin_addr);
			
			for (iit=local_ips; iit && iit->next; iit=iit->next) {}
			if (!iit) {
				local_ips = (struct ip_ll *) malloc(sizeof(struct ip_ll)+strlen(s)+1);
				iit = local_ips;
			} else {
				iit->next = (struct ip_ll *) malloc(sizeof(struct ip_ll)+strlen(s)+1);
				iit = iit->next;
			}
			iit->next = 0;
			
			strcpy(iit->ip, s);
		}
		
		tmp = tmp->ifa_next;
	}
	
	freeifaddrs(addrs);
	
	
	load_list(&host_blacklist, PFW_DATA_DIR "hblack.txt");
	load_list(&host_whitelist, PFW_DATA_DIR "hwhite.txt");
	load_list(&ip_blacklist, PFW_DATA_DIR "ipblack.txt");
	load_list(&ip_whitelist, PFW_DATA_DIR "ipwhite.txt");
	
	load_list(&resolvelist, PFW_DATA_DIR "resolvelist.txt");
	
	load_list(&host_nocachelist, PFW_DATA_DIR "hnocachelist.txt");
	load_list(&ip_nocachelist, PFW_DATA_DIR "ipnocachelist.txt");
	
	
	int i;
// 	for (i=0; i<host_blacklist.length; i++) {
// 		add_ips_to_list(&ip_blacklist, host_blacklist.list[i]);
// 	}
	
// 	for (i=0; i<host_whitelist.length; i++) {
// 		add_ips_to_list(&ip_whitelist, host_whitelist.list[i]);
// 	}
	for (i=0; i<resolvelist.length; i++) {
		add_ips_to_list(&ip_whitelist, resolvelist.list[i]);
	}
	
	print_filter_set(&host_blacklist, "host blacklist");
	print_filter_set(&ip_blacklist, "ip blacklist");
	print_filter_set(&host_whitelist, "host whitelist");
	print_filter_set(&ip_whitelist, "ip blacklist");
	
	
	print_filter_set(&host_nocachelist, "host nocachelist");
	
// 	new_eventgraph(&newp_graph, 0, newp_event_types);
// 	crtx_create_task(newp_graph, 0, "pfw_print_packet", &pfw_print_packet, 0);
// 	
// 	
// 	rcache_host = create_response_cache_task("su", pfw_rcache_create_key_host);
// // 	((struct crtx_cache_task*) rcache_host->userdata)->match_event = &rcache_match_cb_t_regex;
// 	((struct crtx_cache_task*) rcache_host->userdata)->on_add = &host_cache_add_cb;
// 	rcache_host->id = "rcache_host";
// 	
// // 	prefill_rcache( ((struct crtx_cache*) rcache_host->userdata), PFW_DATA_DIR "rcache_host.txt");
// 	add_task(newp_graph, rcache_host);
// 	
// 	
// 	rcache_ip = create_response_cache_task("su", pfw_rcache_create_key_ip);
// // 	((struct crtx_cache_task*) rcache_ip->userdata)->match_event = &rcache_match_cb_t_regex;
// 	((struct crtx_cache_task*) rcache_host->userdata)->on_add = &ip_cache_add_cb;
// 	rcache_ip->id = "rcache_ip";
// 	
// // 	prefill_rcache( ((struct crtx_cache*) rcache_ip->userdata), PFW_DATA_DIR "rcache_ip.txt");
// 	add_task(newp_graph, rcache_ip);
	
// 	crtx_create_task(newp_graph, 0, "pfw_rules_filter", &pfw_rules_filter, 0);
	
// 	print_tasks(newp_graph);
	
	/*
	 * create nfqueue listener
	 */
	
	nfq_list.queue_num = 0;
	nfq_list.default_mark = PFW_DEFAULT;
	
	nfq_list_base = create_listener("nf_queue", &nfq_list);
	if (!nfq_list_base) {
		printf("cannot create nq_queue listener\n");
		return 0;
	}
	
// 	crtx_create_task(nfq_list.parent.graph, 0, "pfw_handler", &pfw_main_handler, &nfq_list);
	
	
	crtx_create_task(nfq_list.parent.graph, 0, "pfw_print_packet", &pfw_print_packet, 0);
	
	rcache_host = create_response_cache_task("su", pfw_rcache_create_key_host);
	((struct crtx_cache_task*) rcache_host->userdata)->on_add = &host_cache_add_cb;
	rcache_host->id = "rcache_host";
	
	add_task(nfq_list.parent.graph, rcache_host);
	
	
	rcache_ip = create_response_cache_task("su", pfw_rcache_create_key_ip);
	((struct crtx_cache_task*) rcache_host->userdata)->on_add = &ip_cache_add_cb;
	rcache_ip->id = "rcache_ip";
	
	add_task(nfq_list.parent.graph, rcache_ip);
	
	crtx_create_task(nfq_list.parent.graph, 0, "pfw_rules_filter", &pfw_rules_filter, 0);
	
	print_tasks(nfq_list.parent.graph);
	
	return 1;
}

void finish() {
	struct ip_ll *ipi, *ipin;
	
	free_response_cache(((struct crtx_cache_task*) rcache_host->userdata)->cache);
	free_response_cache(((struct crtx_cache_task*) rcache_ip->userdata)->cache);
	free_task(rcache_host);
	free_task(rcache_ip);
	
	free_list(&host_blacklist);
	free_list(&host_whitelist);
	free_list(&ip_blacklist);
	free_list(&ip_whitelist);
	free_list(&resolvelist);
	
	free_list(&host_nocachelist);
	free_list(&ip_nocachelist);
	
	for (ipi = local_ips; ipi; ipi=ipin) {
		ipin=ipi->next;
		free(ipi);
	}
	
	free_eventgraph(newp_graph);
	
	free_listener(nfq_list_base);
// 	regfree(&regex);
}