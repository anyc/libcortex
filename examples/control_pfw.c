/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <regex.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>


#include "core.h"
#include "cache.h"
#include "nf_queue.h"
#include "dict.h"
#include "timer.h"

/*
 * To send packets to this tool, add an iptables rule that contains:
 *
 * 		-m mark --mark 0 -j NFQUEUE --queue-num $PFW_QUEUE_NUM
 *
 * It will re-enqueue the processed packet with a changed mark. The mark will
 * be equal to the value in the config file or PFW_DEFAULT_MARK if there is no
 * setting in the config.
 */

/*
 * Example
 * -------
 *
 * With the following commands, the first packet of a new outgoing connection -
 * opened by the user with UID 1000 - will be passed over queue 0 to this tool.
 * You can set a mark 1 or 2 in the config which will reject or accept the
 * connection, respectively. If no item in the config applies, the packet is
 * marked as 2 which causes the connection to be accepted.
 *
 * # PFW_QUEUE_NUM=0 PFW_DEFAULT_MARK=2 ./cortex-pfw
 *
 * iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW -m mark --mark 0 -j NFQUEUE --queue-num 0
 * iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW -m mark --mark 1 -j REJECT
 * iptables -A OUTPUT -m owner --uid-owner 1000 -m state --state NEW -m mark --mark 2 -j ACCEPT
 *
 */

#ifndef PFW_CFG_DIR
#define PFW_CFG_DIR "/etc/cortexd/pfw/"
#endif

// todo
#ifndef PFW_DATA_DIR
#define PFW_DATA_DIR "/var/lib/cortexd/pfw/"
#endif

// listener for the nf_queue
struct crtx_nf_queue_listener nfq_list;
struct crtx_listener_base *nfq_list_base;

// listener for the timer events
struct crtx_timer_listener tlist;
struct crtx_listener_base *blist;
// struct itimerspec newtimer;

// cache tasks that contain the configuration
struct crtx_task *rcache_host;
struct crtx_task *rcache_ip;
struct crtx_task *rcache_resolve;


#define TASK2CACHETASK(task) ((struct crtx_cache_task*) (task)->userdata)

// linked list that contains the local IPs
struct pfw_ip_ll {
	struct pfw_ip_ll *next;
	
	char ip[0];
};
struct pfw_ip_ll *local_ips = 0;

/// update the IP in the given resolve cache entry
static void pfw_update_ip_entry(struct crtx_cache_task *ct, struct crtx_dict_item *entry) {
	struct addrinfo* result;
	struct addrinfo* res;
	int error;
	size_t size;
	char *s;
	struct crtx_dict_item *hostname, *key;
	
	hostname = crtx_get_item(entry->dict, "host");
	
	error = getaddrinfo(hostname->string, NULL, NULL, &result);
	if (error != 0) {
		if (error == EAI_SYSTEM) {
			perror("getaddrinfo");
		} else {
			fprintf(stderr, "error in getaddrinfo(%s): %s\n", hostname->string, gai_strerror(error));
		}
		return;
	}
	
	for (res = result; res != NULL; res = res->ai_next) {
		if (res->ai_family != AF_INET)
			continue;
		
		struct sockaddr_in* saddr = (struct sockaddr_in*)res->ai_addr;
		
		s = inet_ntoa(saddr->sin_addr);
		size = strlen(s);
		
		key = crtx_get_item(entry->dict, "key");
		if (key->string)
			free(key->string);
		
		key->string = crtx_stracpy(s, &size);
		
		if (res->ai_next) {
// 			INFO("TODO multiple resolved IPs per host %s (%s)\n", hostname->string, key->string);
			break;
		}
	}
	
	freeaddrinfo(result);
	return;
}

/// check if $ip is in the list of local IPs
static char pfw_is_ip_local(char *ip) {
	struct pfw_ip_ll *iit;
	
	for (iit=local_ips; iit; iit=iit->next) {
		if (!strcmp(iit->ip, ip))
			return 1;
	}
	
	return 0;
}

/// determine the remote IP and hostname in the given packet dict
static void pfw_get_remote_part(struct crtx_dict *ds, char **remote_ip, char **remote_host) {
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

/// create a key item for the IP cache from the given event structure
static char pfw_rcache_create_key_ip(struct crtx_event *event, struct crtx_dict_item *key) {
	struct crtx_dict *ds;
	char *remote_ip, *remote_host;
	
// 	if (!event->data.dict)
// 		event->data.raw_to_dict(&event->data);
// 	
// 	ds = event->data.dict;
	
	crtx_event_get_payload(event, 0, 0, &ds);
	
	pfw_get_remote_part(ds, &remote_ip, &remote_host);
	
	key->type = 's';
	key->string = crtx_stracpy(remote_ip, 0);
	if (!key->string)
		return 1;
	key->flags |= CRTX_DIF_ALLOCATED_KEY;
	
	return 0;
}

/// create a key item for the hostname cache from the given event structure
static char pfw_rcache_create_key_hostname(struct crtx_event *event, struct crtx_dict_item *key) {
	struct crtx_dict *ds;
	char *remote_ip, *remote_host;
	
// 	if (!event->data.dict)
// 		event->data.raw_to_dict(&event->data);
// 	
// 	ds = event->data.dict;
	
	crtx_event_get_payload(event, 0, 0, &ds);
	
	pfw_get_remote_part(ds, &remote_ip, &remote_host);
	
	if (!remote_host || remote_host[0] == 0)
		return 1;
	
	key->type = 's';
	key->string = crtx_stracpy(remote_host, 0);
	if (!key->string)
		return 1;
	key->flags |= CRTX_DIF_ALLOCATED_KEY;
	
	return 0;
}

static char pfw_on_hit_host(struct crtx_cache_task *rc, struct crtx_dict_item *key, struct crtx_event *event, struct crtx_dict_item *c_entry) {
	char ret, orig_ret;
	
	orig_ret = crtx_cache_on_hit(rc, key, event, c_entry);
	
	/*
	 * The IP cache is configured to not add new entries itself. We add new
	 * entries here, if the host cache entry has no cache_ip property or it
	 * equals 1
	 */
	
	if (c_entry->type == 'D') {
		struct crtx_dict_item ip, *cache_ip;
		
		ip.key = 0;
		cache_ip = crtx_get_item(c_entry->dict, "cache_ip");
		
		if (!cache_ip || CRTX_DICT_GET_NUMBER(cache_ip) == 1) {
// 			struct crtx_dict_item *timeout, *ip_c_entry;
			
			CRTX_DBG("create IP cache entry\n");
			
			// generate key with IP
			ret = pfw_rcache_create_key_ip(event, &ip);
			if (!ret) {
				CRTX_ERROR("cannot create key for IP cache\n");
				return 1;
			}
			
			// add entry to IP cache
			/* ip_c_entry = */ crtx_cache_add_entry(TASK2CACHETASK(rcache_ip)->cache, &ip, event, 0);
// 			timeout = crtx_get_item(c_entry->ds, "timeout");
		}
	} else
		CRTX_ERROR("pfw_on_hit_host received ditem.type %c != 'D'\n", c_entry->type);
	
	return orig_ret;
}

static char pfw_resolve_IPs_timer(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_cache_task *ct;
	struct crtx_dict_item *ditem;
	
	ct = (struct crtx_cache_task *) userdata;
	
	pthread_mutex_lock(&ct->cache->mutex);
	
	ditem = crtx_get_first_item(ct->cache->entries);
	while (ditem) {
		pfw_update_ip_entry(ct, ditem);
		
		ditem = crtx_get_next_item(ct->cache->entries, ditem);
	}
	
	pthread_mutex_unlock(&ct->cache->mutex);
	
// 	crtx_print_dict(ct->cache->entries);
	
	return 1;
}

static char pfw_print_packet(struct crtx_event *event, void *userdata, void **sessiondata) {
	crtx_nfq_print_packet(event->data.dict);
	
	return 1;
}

char pfw_start(unsigned int queue_num, unsigned int default_mark) {
	struct ifaddrs *addrs, *tmp;
	struct pfw_ip_ll *iit;
	char *s;
	int r;
	
	
	getifaddrs(&addrs);
	tmp = addrs;
	
	while (tmp) {
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
		{
			struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
			
			s = inet_ntoa(pAddr->sin_addr);
			
			for (iit=local_ips; iit && iit->next; iit=iit->next) {}
			if (!iit) {
				local_ips = (struct pfw_ip_ll *) malloc(sizeof(struct pfw_ip_ll)+strlen(s)+1);
				iit = local_ips;
			} else {
				iit->next = (struct pfw_ip_ll *) malloc(sizeof(struct pfw_ip_ll)+strlen(s)+1);
				iit = iit->next;
			}
			iit->next = 0;
			
			strcpy(iit->ip, s);
		}
		
		tmp = tmp->ifa_next;
	}
	
	freeifaddrs(addrs);
	
	
	/*
	 * create nfqueue listener
	 */
	
	nfq_list.queue_num = queue_num;
	nfq_list.default_mark = default_mark;
	
	r = crtx_setup_listener("nf_queue", &nfq_list);
	if (r) {
		printf("cannot create nq_queue listener\n");
		return 0;
	}
	nfq_list_base = &nfq_list.base;
	
	
	crtx_create_task(nfq_list.base.graph, 0, "pfw_print_packet", &pfw_print_packet, 0);
	
	{
		// resolved IP
		rcache_resolve = create_response_cache_task("rcache_resolve", pfw_rcache_create_key_ip);
		TASK2CACHETASK(rcache_resolve)->on_add = &crtx_cache_no_add;
		rcache_resolve->id = "rcache_resolve";
		
		crtx_load_cache(TASK2CACHETASK(rcache_resolve)->cache, PFW_DATA_DIR);
		
		add_task(nfq_list.base.graph, rcache_resolve);
	}
	
	{
		/*
		 * from test in timer.c
		 */
		
		// set time for (first) alarm
		tlist.newtimer.it_value.tv_sec = 5;
		tlist.newtimer.it_value.tv_nsec = 0;
		
		// set interval for repeating alarm, set to 0 to disable repetition
		tlist.newtimer.it_interval.tv_sec = 5;
		tlist.newtimer.it_interval.tv_nsec = 0;
		
		tlist.clockid = CLOCK_REALTIME; // clock source, see: man clock_gettime()
		tlist.settime_flags = 0; // absolute (TFD_TIMER_ABSTIME), or relative (0) time, see: man timerfd_settime()
// 		tlist.newtimer = &newtimer;
		
		r = crtx_setup_listener("timer", &tlist);
		if (r) {
			CRTX_ERROR("create_listener(timer) failed\n");
			exit(1);
		}
		blist = &tlist.base;
		
		crtx_create_task(blist->graph, 0, "resolve_ips", pfw_resolve_IPs_timer, TASK2CACHETASK(rcache_resolve));
		
		crtx_start_listener(blist);
	}
	
	{
		// IP
		rcache_ip = create_response_cache_task("rcache_ip", pfw_rcache_create_key_ip);
		TASK2CACHETASK(rcache_ip)->on_add = &crtx_cache_no_add;
		rcache_ip->id = "rcache_ip";
		
		crtx_load_cache(TASK2CACHETASK(rcache_ip)->cache, PFW_DATA_DIR);
		
		add_task(nfq_list.base.graph, rcache_ip);
	}
	
	{
		// host
		rcache_host = create_response_cache_task("rcache_host", pfw_rcache_create_key_hostname);
		TASK2CACHETASK(rcache_host)->on_hit = &pfw_on_hit_host;
		
		rcache_host->id = "rcache_host";
		
		crtx_load_cache(TASK2CACHETASK(rcache_host)->cache, PFW_DATA_DIR);
		
		add_task(nfq_list.base.graph, rcache_host);
	}
	
	crtx_start_listener(nfq_list_base);
	
// 	print_tasks(nfq_list.base.graph);
	
	return 1;
}

char init() {
	unsigned int queue_num, default_mark;
	char *s;
	
	s = getenv("PFW_QUEUE_NUM");
	if (s) {
		queue_num = atoi(s);
	} else
		queue_num = 0;
	
	s = getenv("PFW_DEFAULT_MARK");
	if (s) {
		default_mark = atoi(s);
	} else
		default_mark = 1;
	
	return pfw_start(queue_num, default_mark);
}

void finish() {
	struct pfw_ip_ll *ipi, *ipin;
	
	free_response_cache(((struct crtx_cache_task*) rcache_host->userdata)->cache);
	free_response_cache(((struct crtx_cache_task*) rcache_ip->userdata)->cache);
	free_response_cache(((struct crtx_cache_task*) rcache_resolve->userdata)->cache);
	crtx_free_task(rcache_host);
	crtx_free_task(rcache_ip);
	crtx_free_task(rcache_resolve);
	
	for (ipi = local_ips; ipi; ipi=ipin) {
		ipin=ipi->next;
		free(ipi);
	}
	
	crtx_shutdown_listener(nfq_list_base);
}

#ifdef PFW_STANDALONE
int main(int argc, char **argv) {
	init();
	
	return 0;
}
#endif
