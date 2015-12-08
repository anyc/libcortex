
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <regex.h>

#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <netdb.h>

#include <sys/types.h>
#include <ifaddrs.h>


#include "core.h"
#include "nf_queue.h"
#include "plugins.h"
#include "sd_bus.h"

struct event_task *pfw_handle_task, *rp_cache, *readline_handle_task, *pfw_rules_task;
struct event_graph *rl_graph, *newp_graph, *newp_raw_graph;


// iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW  -m mark --mark 0 -j NFQUEUE --queue-num 0
// iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW -m mark --mark 1 -j REJECT
// iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW -m mark --mark 2 -j ACCEPT

#define PFW_DEFAULT 2
#define PFW_ACCEPT 2
#define PFW_REJECT 1

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

#ifndef PFW_DATA_DIR
#define PFW_DATA_DIR "/etc/cortexd/pfw/"
#endif

struct filter_set ip_whitelist;
struct filter_set ip_blacklist;
struct filter_set host_whitelist;
struct filter_set host_blacklist;




static void pfw_main_handler(struct event *event, void *userdata, void **sessiondata) {
	struct event *newp_raw_event, *newp_event;
	
	if (event->response)
		return;
	
	if (!nfq_packet_msg_okay(event))
		return;
	
	if (newp_raw_graph && newp_raw_graph->tasks) {
		newp_raw_event = new_event();
		newp_raw_event->type = PFW_NEWPACKET_ETYPE;
		newp_raw_event->data = event->data;
		newp_raw_event->data_size = event->data_size;
		
		add_event(newp_graph, newp_raw_event);
		
		wait_on_event(newp_raw_event);
	}
	
	if (newp_graph && newp_graph->tasks) {
		int res;
		char host[256];
// 		char *s;
// 		char buf[64];
		struct ev_nf_queue_packet_msg *ev;
		struct iphdr *iph;
		size_t size;
		char *sign;
		struct data_item *di;
		struct data_struct *ds;
		
		
		ev = (struct ev_nf_queue_packet_msg*) event->data;
		iph = (struct iphdr*)ev->payload;
		
		
		sign = PFW_NEWPACKET_SIGNATURE;
		size = strlen(sign)*sizeof(struct data_item) + sizeof(struct data_struct);
		ds = (struct data_struct*) malloc(size);
		
		ds->signature = sign;
		
		di = &ds->items[PFW_NEWP_PROTOCOL];
		di->type = 'u';
		di->size = 0;
		di->key = "protocol";
		di->uint32 = iph->protocol;
// 		di->flags = DIF_DATA_UNALLOCATED;
// 		di->string = nfq_proto2str(iph->protocol);
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
		di->key = "src_hostname";
		di->string = stracpy(host, &di->size);
		di->size += 1; // add one byte for terminating \0
		
		di = &ds->items[PFW_NEWP_SRC_IP];
		di->type = 's';
		di->size = 0;
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
		di->key = "dst_hostname";
		di->string = stracpy(host, &di->size);
		di->size += 1; // add one byte for terminating \0
		
		di = &ds->items[PFW_NEWP_DST_IP];
		di->type = 's';
		di->size = 0;
		di->key = "dst_ip";
		di->string = stracpy(inet_ntoa(*(struct in_addr *)&iph->daddr), &di->size);
		di->size += 1; // add one byte for terminating \0
		
		di = &ds->items[PFW_NEWP_PAYLOAD];
		di->type = 'D';
		di->key = "payload";
		
		ds->items[PFW_NEWP_PAYLOAD].flags = DIF_LAST;
		
		if (iph->protocol == 6) {
			struct data_item *pdi;
			size_t size;
			char *sign;
			
			struct tcphdr *tcp = (struct tcphdr *) (ev->payload + (iph->ihl << 2));
			
			sign = PFW_NEWPACKET_TCP_SIGNATURE;
			size = strlen(sign)*sizeof(struct data_item) + sizeof(struct data_struct);
			di->ds = (struct data_struct*) malloc(size);
			
			di->ds->signature = sign;
			
			pdi = &di->ds->items[PFW_NEWP_TCP_SPORT];
			pdi->type = 'u';
			pdi->key = "src_port";
			pdi->uint32 = ntohs(tcp->source);
			
			pdi = &di->ds->items[PFW_NEWP_TCP_DPORT];
			pdi->type = 'u';
			pdi->key = "dst_port";
			pdi->uint32 = ntohs(tcp->dest);
			
			di->ds->items[PFW_NEWP_TCP_DPORT].flags = DIF_LAST;
			
// 			fprintf(stdout, "TCP{sport=%u; dport=%u; seq=%u; ack_seq=%u; flags=u%ua%up%ur%us%uf%u; window=%u; urg=%u}\n",
// 				   ntohs(tcp->source), ntohs(tcp->dest), ntohl(tcp->seq), ntohl(tcp->ack_seq)
// 				   ,tcp->urg, tcp->ack, tcp->psh, tcp->rst, tcp->syn, tcp->fin, ntohs(tcp->window), tcp->urg_ptr);
		}
		
		if (iph->protocol == 17) {
			struct data_item *pdi;
			size_t size;
			char *sign;
			
			struct udphdr *udp = (struct udphdr *) (ev->payload + (iph->ihl << 2));
			
			sign = PFW_NEWPACKET_UDP_SIGNATURE;
			size = strlen(sign)*sizeof(struct data_item) + sizeof(struct data_struct);
			di->ds = (struct data_struct*) malloc(size);
			
			di->ds->signature = sign;
			
			pdi = &di->ds->items[PFW_NEWP_UDP_SPORT];
			pdi->type = 'u';
			pdi->key = "src_port";
			pdi->uint32 = ntohs(udp->source);
			
			pdi = &di->ds->items[PFW_NEWP_UDP_DPORT];
			pdi->type = 'u';
			pdi->key = "dst_port";
			pdi->uint32 = ntohs(udp->dest);
			
			di->ds->items[PFW_NEWP_UDP_DPORT].flags = DIF_LAST;
			
			// 			fprintf(stdout,"UDP{sport=%u; dport=%u; len=%u}\n",
			// 				   ntohs(udp->source), ntohs(udp->dest), udp->len);
		}
		
		
		newp_event = new_event();
		newp_event->type = PFW_NEWPACKET_ETYPE;
		newp_event->data = ds;
		newp_event->data_size = size;
		
		add_event(newp_graph, newp_event);
		
		wait_on_event(newp_event);
		
		if (newp_event->response)
			event->response = newp_event->response;
	}
	
	if (!event->response) {
		printf("set to %d\n", PFW_DEFAULT);
		event->response = (void*) PFW_DEFAULT;
	}
}

static void readline_handler(struct event *event, void *userdata, void **sessiondata) {
	struct event *rl_event;
	int res;
	char host[64];
	char *s;
	char buf[64];
	struct ev_nf_queue_packet_msg *ev;
	struct iphdr *iph;
	
	
	ev = (struct ev_nf_queue_packet_msg*) event->data;
	iph = (struct iphdr*)ev->payload;
	
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr = *(struct in_addr *)&iph->saddr;
	res = getnameinfo((struct sockaddr*) &sa, sizeof(struct sockaddr), host, 64, 0, 0, 0);
	if (res) {
		printf("getnameinfo failed %d\n", res);
		host[0] = 0;
	}
	
	s = buf;
	s += sprintf(s, "%s %s (%s) ", nfq_proto2str(iph->protocol),
			   host,
		    inet_ntoa(*(struct in_addr *)&iph->saddr));
	
	sa.sin_addr = *(struct in_addr *)&iph->daddr;
	res = getnameinfo((struct sockaddr*) &sa, sizeof(struct sockaddr), host, 64, 0, 0, 0);
	if (res) {
		printf("getnameinfo failed %d\n", res);
		host[0] = 0;
	}
	
	s += sprintf(s, "%s (%s) (yes/no)",
			   host,
		    inet_ntoa(*(struct in_addr *)&iph->daddr));
	
	
	while (!event->response) {
		rl_event = new_event();
		rl_event->type = "readline/request";
		rl_event->data = buf;
		rl_event->data_size = s-buf;
		
		if (!rl_graph)
			rl_graph = get_graph_for_event_type("readline/request");
		
		add_event(rl_graph, rl_event);
		
		wait_on_event(rl_event);
		
		if (rl_event->response && !strncmp(rl_event->response, "yes", 3)) {
			event->response = (void*) PFW_ACCEPT;
		}
		if (rl_event->response && !strncmp(rl_event->response, "no", 2)) {
			event->response = (void*) PFW_REJECT;
		}
	}
}

char * nfq_decision_cache_create_key(struct event *event) {
	struct ev_nf_queue_packet_msg *ev;
	struct iphdr *iph;
	char *ss, *sd, *s;
	size_t len;
	
	if (!nfq_packet_msg_okay(event))
		return 0;
	
	ev = (struct ev_nf_queue_packet_msg*) event->data;
	
	iph = (struct iphdr*)ev->payload;
	
	ss = inet_ntoa(*(struct in_addr *)&iph->saddr);
	sd = inet_ntoa(*(struct in_addr *)&iph->daddr);
	
	ASSERT(iph->protocol < 999);
	len = 3 + 1 + strlen(ss) + 1 + strlen(sd) + 1;
	s = (char*) malloc(len);
	sprintf(s, "%u %s %s", iph->protocol, ss, sd);
	
	return s;
}

// struct data_item *get_data_item(struct data_struct *ds, char *key) {
// 	struct data_item *di;
// 	char *c;
// 	
// 	c = ds->signature;
// 	di = ds->items;
// 	while (c && *c) {
// 		if (!strcmp(di->key, key))
// 			return di;
// 		c++;
// 		di++;
// 	}
// 	
// 	return 0;
// }

struct data_item *get_data_item(struct data_struct *ds, char *key) {
	struct data_item *di;
	
	di = ds->items;
	while (di && DIF_IS_LAST_ITEM(di)) {
		if (!strcmp(di->key, key))
			return di;
		
		di++;
	}
	
	return 0;
}

char pfw_is_ip_local(char *ip) {
	struct ip_ll *iit;
	
	for (iit=local_ips; iit; iit=iit->next) {
		if (!strcmp(iit->ip, ip))
			return 1;
	}
	
	return 0;
}

static void pfw_print_packet(struct event *event, void *userdata, void **sessiondata) {
	struct data_struct *ds, *pds;
	char src_local, dst_local;
	
	ds = (struct data_struct *) event->data;
	
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
	
	printf("%s ", nfq_proto2str(ds->items[PFW_NEWP_PROTOCOL].uint32));
	
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


void load_list(struct filter_set *fset, char *file) {
	FILE *f;
	int ret;
	char buf[1024];
	
	f = fopen(file, "r");
	
	if (!f)
		return;
	
	while (fgets(buf, sizeof(buf), f)) {
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

static void pfw_rules_filter(struct event *event, void *userdata, void **sessiondata) {
	struct data_struct *ds;
	char src_local, dst_local;
	unsigned int remote_ip, remote_host;
	
	ds = (struct data_struct *) event->data;
	
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
		remote_ip = PFW_NEWP_SRC_IP;
		remote_host = PFW_NEWP_SRC_HOST;
	} else {
		if (src_local) {
			remote_ip = PFW_NEWP_DST_IP;
			remote_host = PFW_NEWP_DST_HOST;
		} else {
			remote_ip = PFW_NEWP_SRC_IP;
			remote_host = PFW_NEWP_SRC_HOST;
		}
	}
	
	if (match_regexp_list(ds->items[remote_host].string, &host_blacklist)) {
		event->response = (void *) PFW_REJECT;
		return;
	}
	
	if (match_regexp_list(ds->items[remote_ip].string, &ip_blacklist)) {
		event->response = (void *) PFW_REJECT;
		return;
	}
	
	if (match_regexp_list(ds->items[remote_host].string, &host_whitelist)) {
		event->response = (void *) PFW_REJECT;
		return;
	}
	
	if (match_regexp_list(ds->items[remote_ip].string, &ip_whitelist)) {
		event->response = (void *) PFW_REJECT;
		return;
	}
	
}

void init() {
	struct nfq_thread_data *td;
	
	td = (struct nfq_thread_data*) create_listener("nf_queue", 0);
	
	rp_cache = create_response_cache_task(&nfq_decision_cache_create_key);
	add_task(td->graph, rp_cache);
	
	new_eventgraph(&newp_graph, newp_event_types);
// 	new_eventgraph(&newp_raw_graph, newp_raw_event_types);
	
	pfw_handle_task = new_task();
	pfw_handle_task->id = "pfw_handler";
	pfw_handle_task->handle = &pfw_main_handler;
	pfw_handle_task->userdata = td;
	add_task(td->graph, pfw_handle_task);
	
	
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
	
// 	readline_handle_task = new_task();
// 	readline_handle_task->id = "readline_handler";
// 	readline_handle_task->handle = &readline_handler;
// 	add_task(newp_graph, readline_handle_task);
// 	
	
// 	int i;
// 	
// 	for (i=0; i < n_host_blacklist; i++) {
// 		reti = regcomp(&rhost_blacklist[i], host_blacklist[i], 0);
// 		if (reti) {
// 			fprintf(stderr, "Could not compile regex %s\n", host_blacklist[i]);
// 			exit(1);
// 		}
// 	}
// 	
// 	for (i=0; i < n_host_whitelist; i++) {
// 		reti = regcomp(&rhost_whitelist[i], host_whitelist[i], 0);
// 		if (reti) {
// 			fprintf(stderr, "Could not compile regex %s\n", host_whitelist[i]);
// 			exit(1);
// 		}
// 	}
	
	load_list(&host_blacklist, PFW_DATA_DIR "hblack.txt");
	load_list(&host_whitelist, PFW_DATA_DIR "hwhite.txt");
	load_list(&ip_blacklist, PFW_DATA_DIR "ipblack.txt");
	load_list(&ip_whitelist, PFW_DATA_DIR "ipwhite.txt");
	
	pfw_rules_task = new_task();
	pfw_rules_task->id = "pfw_print_packet";
	pfw_rules_task->handle = &pfw_print_packet;
	add_task(newp_graph, pfw_rules_task);
	
	pfw_rules_task = new_task();
	pfw_rules_task->id = "pfw_rules_filter";
	pfw_rules_task->handle = &pfw_rules_filter;
	add_task(newp_graph, pfw_rules_task);
}

void finish() {
// 	regfree(&regex);
}