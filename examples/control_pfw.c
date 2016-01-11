
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <ctype.h>

#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <ifaddrs.h>


#include "core.h"
#include "cache.h"
#include "nf_queue.h"
#include "controls.h"

struct crtx_nfq_listener nfq_list;
struct crtx_listener_base *nfq_list_base;
struct crtx_task *pfw_handle_task, *rp_cache, *readline_handle_task, *pfw_rules_task;
struct crtx_graph *rl_graph, *newp_graph, *newp_raw_graph;
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

#define PFW_NEWPACKET_SIGNATURE "uststD"
enum {PFW_NEWP_PROTOCOL=0, PFW_NEWP_SRC_IP, PFW_NEWP_SRC_HOST, PFW_NEWP_DST_IP, PFW_NEWP_DST_HOST, PFW_NEWP_PAYLOAD};

#define PFW_NEWPACKET_TCP_SIGNATURE "uu"
enum {PFW_NEWP_TCP_SPORT=0, PFW_NEWP_TCP_DPORT};

#define PFW_NEWPACKET_UDP_SIGNATURE "uu"
enum {PFW_NEWP_UDP_SPORT=0, PFW_NEWP_UDP_DPORT};

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

static void pfw_main_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event *newp_raw_event, *newp_event;
	struct crtx_nfq_packet *ev;
	
// 	if (event->response.raw)
// 		return;
	
	if (event->data.raw_size < sizeof(struct crtx_nfq_packet))
		return;
	
	ev = (struct crtx_nfq_packet*) event->data.raw;
	
	// if someone set a response already, ignore
	if (event->response.raw && event->response.raw != &ev->mark_out)
		return;
	
	ev->mark_out = PFW_DEFAULT;
	
	if (newp_raw_graph && newp_raw_graph->tasks) {
		newp_raw_event = create_event(PFW_NEWPACKET_ETYPE, event->data.raw, event->data.raw_size);
		
		add_event(newp_graph, newp_raw_event);
		
		wait_on_event(newp_raw_event);
		
		free_event(newp_raw_event);
	}
	
	if (newp_graph && newp_graph->tasks) {
		int res;
		char host[256];
		struct iphdr *iph;
		size_t size;
		char *sign;
		struct crtx_dict_item *di;
		struct crtx_dict *ds;
		
		iph = (struct iphdr*)ev->payload;
		
		
		sign = PFW_NEWPACKET_SIGNATURE;
		size = strlen(sign)*sizeof(struct crtx_dict_item) + sizeof(struct crtx_dict);
		ds = (struct crtx_dict*) malloc(size);
		
		ds->signature = sign;
		
		di = &ds->items[PFW_NEWP_PROTOCOL];
		di->type = 'u';
		di->size = 0;
		di->flags = 0;
		di->key = "protocol";
		di->uint32 = iph->protocol;
// 		di->flags = DIF_DATA_UNALLOCATED;
// 		di->string = crtx_nfq_proto2str(iph->protocol);
// 		di->size = strlen(di->string)+1; // add one byte for terminating \0
		
		
		struct sockaddr_in sa;
		sa.sin_family = AF_INET;
		sa.sin_addr = *(struct in_addr *)&iph->saddr;
		res = getnameinfo((struct sockaddr*) &sa, sizeof(struct sockaddr), host, 64, 0, 0, 0);
		if (res) {
			printf("getnameinfo failed %d\n", res);
			host[0] = 0;
		}
		
		di = &ds->items[PFW_NEWP_SRC_HOST];
		di->type = 's';
		di->size = 0;
		di->flags = 0;
		di->key = "src_hostname";
		di->string = stracpy(host, &di->size);
		di->size += 1; // add one byte for terminating \0
		
		di = &ds->items[PFW_NEWP_SRC_IP];
		di->type = 's';
		di->size = 0;
		di->flags = 0;
		di->key = "src_ip";
		di->string = stracpy(inet_ntoa(*(struct in_addr *)&iph->saddr), &di->size);
		di->size += 1; // add one byte for terminating \0
		
		sa.sin_addr = *(struct in_addr *)&iph->daddr;
		res = getnameinfo((struct sockaddr*) &sa, sizeof(struct sockaddr), host, 64, 0, 0, 0);
		if (res) {
			printf("getnameinfo failed %d\n", res);
			host[0] = 0;
		}
		
		di = &ds->items[PFW_NEWP_DST_HOST];
		di->type = 's';
		di->size = 0;
		di->flags = 0;
		di->key = "dst_hostname";
		di->string = stracpy(host, &di->size);
		di->size += 1; // add one byte for terminating \0
		
		di = &ds->items[PFW_NEWP_DST_IP];
		di->type = 's';
		di->size = 0;
		di->flags = 0;
		di->key = "dst_ip";
		di->string = stracpy(inet_ntoa(*(struct in_addr *)&iph->daddr), &di->size);
		di->size += 1; // add one byte for terminating \0
		
		di = &ds->items[PFW_NEWP_PAYLOAD];
		di->type = 'D';
		di->flags = 0;
		di->key = "payload";
		di->ds = 0;
		
		ds->items[PFW_NEWP_PAYLOAD].flags = DIF_LAST;
		
		if (iph->protocol == 6) {
			struct crtx_dict_item *pdi;
			size_t size;
			char *sign;
			
			struct tcphdr *tcp = (struct tcphdr *) (ev->payload + (iph->ihl << 2));
			
			sign = PFW_NEWPACKET_TCP_SIGNATURE;
			size = strlen(sign)*sizeof(struct crtx_dict_item) + sizeof(struct crtx_dict);
			di->ds = (struct crtx_dict*) malloc(size);
			
			di->ds->signature = sign;
			
			pdi = &di->ds->items[PFW_NEWP_TCP_SPORT];
			pdi->type = 'u';
			pdi->flags = 0;
			pdi->key = "src_port";
			pdi->uint32 = ntohs(tcp->source);
			
			pdi = &di->ds->items[PFW_NEWP_TCP_DPORT];
			pdi->type = 'u';
			pdi->flags = 0;
			pdi->key = "dst_port";
			pdi->uint32 = ntohs(tcp->dest);
			
			di->ds->items[PFW_NEWP_TCP_DPORT].flags = DIF_LAST;
			
// 			fprintf(stdout, "TCP{sport=%u; dport=%u; seq=%u; ack_seq=%u; flags=u%ua%up%ur%us%uf%u; window=%u; urg=%u}\n",
// 				   ntohs(tcp->source), ntohs(tcp->dest), ntohl(tcp->seq), ntohl(tcp->ack_seq)
// 				   ,tcp->urg, tcp->ack, tcp->psh, tcp->rst, tcp->syn, tcp->fin, ntohs(tcp->window), tcp->urg_ptr);
		} else
		if (iph->protocol == 17) {
			struct crtx_dict_item *pdi;
			size_t size;
			char *sign;
			
			struct udphdr *udp = (struct udphdr *) (ev->payload + (iph->ihl << 2));
			
			sign = PFW_NEWPACKET_UDP_SIGNATURE;
			size = strlen(sign)*sizeof(struct crtx_dict_item) + sizeof(struct crtx_dict);
			di->ds = (struct crtx_dict*) malloc(size);
			
			di->ds->signature = sign;
			
			pdi = &di->ds->items[PFW_NEWP_UDP_SPORT];
			pdi->type = 'u';
			pdi->flags = 0;
			pdi->key = "src_port";
			pdi->uint32 = ntohs(udp->source);
			
			pdi = &di->ds->items[PFW_NEWP_UDP_DPORT];
			pdi->type = 'u';
			pdi->flags = 0;
			pdi->key = "dst_port";
			pdi->uint32 = ntohs(udp->dest);
			
			di->ds->items[PFW_NEWP_UDP_DPORT].flags = DIF_LAST;
			
			// 			fprintf(stdout,"UDP{sport=%u; dport=%u; len=%u}\n",
			// 				   ntohs(udp->source), ntohs(udp->dest), udp->len);
		}
		
		
// 		newp_event = new_event();
// 		newp_event->type = PFW_NEWPACKET_ETYPE;
// 		newp_event->data = ds;
// 		newp_event->data_size = size;
		newp_event = create_event(PFW_NEWPACKET_ETYPE, 0, 0);
		newp_event->data.dict = ds;
		newp_event->data.raw = event->data.raw;
		newp_event->data.raw_size = event->data.raw_size;
		newp_event->response.copy_raw = &crtx_copy_raw_data;
		
// 		newp_event->response.raw = event->response.raw;
// 		newp_event->response.raw_size = event->response.raw_size;
// 		newp_event->response.copy_raw = &crtx_copy_raw_data;
		
		reference_event_release(newp_event);
		
		add_event(newp_graph, newp_event);
		
		wait_on_event(newp_event);
		
// 		if (newp_event->response.raw) {
// 			event->response.raw = newp_event->response.raw;
// 			((struct crtx_nfq_packet*) event->data.raw)->mark_out = newp_event->response.raw;
// 			SET_MARK(event, (u_int32_t) (uintptr_t) newp_event->response.raw);
// 			newp_event->response.raw = 0;
// 		}
		
		if (newp_event->response.raw && newp_event->response.raw != &ev->mark_out)
			ev->mark_out = *((u_int32_t*) newp_event->response.raw);
		
		printf("asd %d\n", ev->mark_out);
		
		// do not free as referenced data does not belong to us
		newp_event->data.raw = 0; 
		newp_event->response.raw = 0;
		dereference_event_release(newp_event);
		
// 		free_dict(ds);
		
// 		free_event(newp_event);
	}
	
// 	if (!event->response.raw) {
// 		printf("set to %d\n", PFW_DEFAULT);
// 		event->response.raw = (void*) PFW_DEFAULT;
// 		((struct crtx_nfq_packet*) event->data.raw)->mark_out = PFW_DEFAULT;
// 	}
}

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

char * pfw_rcache_create_key_ip(struct crtx_event *event) {
	struct crtx_dict *ds;
	char *remote_ip, *remote_host;
	
	ds = event->data.dict;
	
	pfw_get_remote_part(ds, &remote_ip, &remote_host);
	
	return stracpy(remote_ip, 0);
}

char * pfw_rcache_create_key_host(struct crtx_event *event) {
	struct crtx_dict *ds;
	char *remote_ip, *remote_host;
	
	ds = event->data.dict;
	
	pfw_get_remote_part(ds, &remote_ip, &remote_host);
	
	return stracpy(remote_host, 0);
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
	
// 	if (event->response.raw)
// 		printf("asdD %d\n", *((u_int32_t*) event->response.raw));
	if (event->response.raw)
		return;
	
// 	ds = (struct crtx_dict *) event->data;
	ds = event->data.dict;
	
	pfw_get_remote_part(ds, &remote_ip, &remote_host);
	
// 	#define SET_MARK(event, mark) { *((u_int32_t*) (event)->data.raw) = (mark); event->response.raw = & }
	#define SET_MARK(event, mark) { \
			(event)->response.raw = &((struct crtx_nfq_packet*) (event)->data.raw)->mark_out; \
			(event)->response.raw_size = sizeof(u_int32_t); \
			((struct crtx_nfq_packet*) (event)->data.raw)->mark_out = (mark); \
		}
	if (match_regexp_list(remote_host, &host_blacklist)) {
// 		event->response.raw = (void *) PFW_REJECT;
		SET_MARK(event, PFW_REJECT);
		return;
	}
	
	if (match_regexp_list(remote_ip, &ip_blacklist)) {
// 		event->response.raw = (void *) PFW_REJECT;
		SET_MARK(event, PFW_REJECT);
		return;
	}
	
	if (match_regexp_list(remote_host, &host_whitelist)) {
// 		event->response.raw = (void *) PFW_ACCEPT;
		SET_MARK(event, PFW_ACCEPT);
		return;
	}
	
	if (match_regexp_list(remote_ip, &ip_whitelist)) {
// 		event->response.raw = (void *) PFW_ACCEPT;
		SET_MARK(event, PFW_ACCEPT);
		return;
	}
	
}

void init() {
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
// 			printf("%s\n", iit->ip);
// 			printf("%s: %s\n", tmp->ifa_name, inet_ntoa(pAddr->sin_addr));
		}
		
		tmp = tmp->ifa_next;
	}
	
	freeifaddrs(addrs);
	
	
	load_list(&host_blacklist, PFW_DATA_DIR "hblack.txt");
	load_list(&host_whitelist, PFW_DATA_DIR "hwhite.txt");
	load_list(&ip_blacklist, PFW_DATA_DIR "ipblack.txt");
	load_list(&ip_whitelist, PFW_DATA_DIR "ipwhite.txt");
	
	load_list(&resolvelist, PFW_DATA_DIR "resolvelist.txt");
	
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
	
	
	new_eventgraph(&newp_graph, newp_event_types);
	
	pfw_rules_task = new_task();
	pfw_rules_task->id = "pfw_print_packet";
	pfw_rules_task->handle = &pfw_print_packet;
	add_task(newp_graph, pfw_rules_task);
	
	
	rcache_host = create_response_cache_task("su", pfw_rcache_create_key_host);
	((struct crtx_cache_task*) rcache_host->userdata)->match_event = &rcache_match_cb_t_regex;
	rcache_host->id = "rcache_host";
	
// 	prefill_rcache( ((struct crtx_cache*) rcache_host->userdata), PFW_DATA_DIR "rcache_host.txt");
	add_task(newp_graph, rcache_host);
	
	
	rcache_ip = create_response_cache_task("su", pfw_rcache_create_key_ip);
	((struct crtx_cache_task*) rcache_ip->userdata)->match_event = &rcache_match_cb_t_regex;
	rcache_ip->id = "rcache_ip";
	
// 	prefill_rcache( ((struct crtx_cache*) rcache_ip->userdata), PFW_DATA_DIR "rcache_ip.txt");
	add_task(newp_graph, rcache_ip);
	
	pfw_rules_task = new_task();
	pfw_rules_task->id = "pfw_rules_filter";
	pfw_rules_task->handle = &pfw_rules_filter;
	add_task(newp_graph, pfw_rules_task);
	
	print_tasks(newp_graph);
	
	/*
	 * create nfqueue listener
	 */
	
	nfq_list.queue_num = 0;
	nfq_list.default_mark = PFW_DEFAULT;
	
	nfq_list_base = create_listener("nf_queue", &nfq_list);
	if (!nfq_list_base) {
		printf("cannot create nq_queue listener\n");
		return;
	}
	
	pfw_handle_task = new_task();
	pfw_handle_task->id = "pfw_handler";
	pfw_handle_task->handle = &pfw_main_handler;
	pfw_handle_task->userdata = &nfq_list;
	add_task(nfq_list.parent.graph, pfw_handle_task);
}

void finish() {
	struct ip_ll *ipi, *ipin;
	
	free_task(pfw_rules_task);
	
	free_response_cache(((struct crtx_cache*) rcache_host->userdata));
	free_response_cache(((struct crtx_cache*) rcache_ip->userdata));
	free_task(rcache_host);
	free_task(rcache_ip);
	
	free_list(&host_blacklist);
	free_list(&host_whitelist);
	free_list(&ip_blacklist);
	free_list(&ip_whitelist);
	free_list(&resolvelist);
	
	for (ipi = local_ips; ipi; ipi=ipin) {
		ipin=ipi->next;
		free(ipi);
	}
	
	free_task(pfw_handle_task);
	
	free_eventgraph(newp_graph);
	
// 	free_nf_queue_listener(td);
	free_listener(nfq_list_base);
// 	regfree(&regex);
}