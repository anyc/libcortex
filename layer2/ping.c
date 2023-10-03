/*
 * Mario Kicherer (dev@kicherer.org) 2023
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <netinet/ip.h>
#include <linux/icmp.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "intern.h"
#include "ping.h"

#define PACKET_LEN 64

enum icmp_type { ICMP_T_ECHO, ICMP_T_GATEWAY, ICMP_T_FRAG, ICMP_T_RESERVED};

struct crtx_dict *crtx_ping_raw2dict_addr(struct icmphdr *ptr, enum icmp_type icmp_type) {
	struct crtx_dict *dict;
	struct crtx_dict_item *di;
	
	dict = crtx_init_dict(0, 0, 0);
	
	crtx_dict_new_item(dict, 'u', "type", ptr->type, sizeof(ptr->type), 0);
	
	crtx_dict_new_item(dict, 'u', "code", ptr->code, sizeof(ptr->code), 0);
	
	crtx_dict_new_item(dict, 'u', "checksum", ptr->checksum, sizeof(ptr->checksum), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'D', "un", 0, 0, 0);
	di->dict = crtx_init_dict(0, 0, 0);
	
	if (icmp_type == ICMP_T_ECHO) {
		struct crtx_dict_item *di2;
		
		di2 = crtx_alloc_item(di->dict);
		crtx_fill_data_item(di2, 'D', "echo", 0, 0, 0);
		di2->dict = crtx_init_dict(0, 0, 0);
		
		crtx_dict_new_item(di2->dict, 'u', "id", ntohs(ptr->un.echo.id), sizeof(ptr->un.echo.id), 0);
		crtx_dict_new_item(di2->dict, 'u', "sequence", ntohs(ptr->un.echo.sequence), sizeof(ptr->un.echo.sequence), 0);
	}
	
	if (icmp_type == ICMP_T_GATEWAY) {
		crtx_dict_new_item(di->dict, 'u', "gateway", ntohs(ptr->un.gateway), sizeof(ptr->un.gateway), 0);
	}
	
	if (icmp_type == ICMP_T_FRAG) {
		struct crtx_dict_item *di2;
		
		di2 = crtx_alloc_item(di->dict);
		crtx_fill_data_item(di2, 'D', "frag", 0, 0, 0);
		di2->dict = crtx_init_dict(0, 0, 0);
		
		crtx_dict_new_item(dict, 'u', "__unused", ptr->un.frag.__unused, sizeof(ptr->un.frag.__unused), 0);
		crtx_dict_new_item(dict, 'u', "mtu", ntohs(ptr->un.frag.mtu), sizeof(ptr->un.frag.mtu), 0);
	}

	if (icmp_type == ICMP_T_RESERVED) {
		struct crtx_dict_item *di2;
		
		di2 = crtx_alloc_item(di->dict);
		crtx_fill_data_item(di2, 'D', "reserved", 0, 0, 0);
		
		di2->dict = crtx_init_dict("uuuu", 4, 0);
		
		crtx_fill_data_item(&di2->dict->items[0], 'u', 0, ptr->un.reserved[0], sizeof(ptr->un.reserved[0]), 0);
		crtx_fill_data_item(&di2->dict->items[1], 'u', 0, ptr->un.reserved[1], sizeof(ptr->un.reserved[1]), 0);
		crtx_fill_data_item(&di2->dict->items[2], 'u', 0, ptr->un.reserved[2], sizeof(ptr->un.reserved[2]), 0);
		crtx_fill_data_item(&di2->dict->items[3], 'u', 0, ptr->un.reserved[3], sizeof(ptr->un.reserved[3]), 0);
	}
	
	return dict;
}

// copied from in_cksum() iputils/clockdiff.c (BSD-3 license)
static inline int addcarry(int sum)
{
	if (sum & 0xffff0000) {
		sum &= 0xffff;
		sum++;
	}
	return sum;
}
static int in_cksum(unsigned short *addr, int len)
{
	union word {
		char c[2];
		unsigned short s;
	} u;
	int sum = 0;
	
	while (len > 0) {
		/* add by words */
		while ((len -= 2) >= 0) {
			if ((unsigned long)addr & 0x1) {
				/* word is not aligned */
				u.c[0] = *(char *)addr;
				u.c[1] = *((char *)addr + 1);
				sum += u.s;
				addr++;
			} else
				sum += *addr++;
			sum = addcarry(sum);
		}
		if (len == -1)
			/* odd number of bytes */
			u.c[0] = *(unsigned char *)addr;
	}
	if (len == -1) {
		/*
		 * The last mbuf has odd # of bytes.  Follow the standard (the odd byte
		 * is shifted left by 8 bits)
		 */
		u.c[1] = 0;
		sum += u.s;
		sum = addcarry(sum);
	}
	
	return (~sum & 0xffff);
}

static char ping_timer_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_ping_listener *ping_lstnr;
	ssize_t ssize;
	struct icmphdr *icmp;
	
	
	ping_lstnr = (struct crtx_ping_listener *) userdata;
	
	if (!ping_lstnr->packet) {
		ping_lstnr->packet = (unsigned char *) calloc(1, PACKET_LEN);
		if (!ping_lstnr->packet) {
			CRTX_ERROR("malloc ping packet failed\n");
			return 1;
		}
	}
	
	icmp = (struct icmphdr *)ping_lstnr->packet;
	icmp->type = ICMP_ECHO;
	icmp->code = 0;
	icmp->checksum = 0;
	icmp->un.echo.id = htons(ping_lstnr->echo_id);
	icmp->un.echo.sequence = htons(ping_lstnr->packets_sent);
	ping_lstnr->packets_sent += 1;
	
	icmp->checksum = in_cksum((unsigned short *)icmp, PACKET_LEN);
	
	ssize = sendto(crtx_listener_get_fd(&ping_lstnr->base), ping_lstnr->packet, PACKET_LEN, 0, (struct sockaddr*)&ping_lstnr->dst, sizeof(struct sockaddr_in));
	if (ssize != PACKET_LEN) {
		CRTX_ERROR("ping: sendto failed: %zd != %d\n", ssize, PACKET_LEN);
		return 1;
	}
	
	CRTX_DBG("ping tx %d\n", ntohs(icmp->un.echo.sequence));
	
	return 0;
}

static int send_event(struct crtx_ping_listener *ping_lstnr, int type, struct icmphdr *data, size_t size) {
	struct crtx_event *new_event;
	void *copy;
	
	crtx_create_event(&new_event);
	
	copy = malloc(size);
	memcpy(copy, data, size);
	
	new_event->type = type;
	crtx_event_set_raw_data(new_event, 'p', (char*) copy, size, 0);
	
	crtx_add_event(ping_lstnr->base.graph, new_event);
	
	return 0;
}

static char fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_ping_listener *ping_lstnr;
	ssize_t ssize;
	char packet[1024];
	struct sockaddr_in src;
	socklen_t src_len;
	struct icmphdr *icmp, *orig_icmp;
	int checksum, pkt_checksum;
	
	
	ping_lstnr = (struct crtx_ping_listener *) userdata;
	
	src_len = sizeof(src);
	ssize = recvfrom(crtx_listener_get_fd(&ping_lstnr->base), &packet, sizeof(packet), 0,
			 (struct sockaddr*)&src, &src_len);
	if (ssize <= 0 || ssize < sizeof(struct ip) + sizeof(struct icmphdr)) {
		CRTX_ERROR("ping: recvfrom failed: %s %zd < %zu\n", strerror(errno), ssize, sizeof(struct ip) + sizeof(struct icmphdr));
		
		if (ssize > 0) {
			int i;
			for (i=0;i<ssize; i++)
				CRTX_DBG("%02hhx ", packet[i]);
			CRTX_DBG("\n");
		}
		
		return 0;
	}
	
	icmp = (struct icmphdr *) (packet + sizeof(struct ip));
	
	checksum = icmp->checksum;
	icmp->checksum = 0;
	
	pkt_checksum = in_cksum((unsigned short *)icmp, ssize - sizeof(struct ip));
	if (checksum != pkt_checksum) {
		if (ssize > 0) {
			int i;
			for (i=0;i<ssize; i++)
				CRTX_ERROR("%02hhx ", packet[i]);
			CRTX_ERROR("\n");
		}
		CRTX_ERROR("received invalid ICMP packet: %d != %d\n", checksum, pkt_checksum);
		return 0;
	}
	
	if (icmp->type != ICMP_ECHOREPLY) {
		if (icmp->type == ICMP_DEST_UNREACH) {
			if (ssize >= sizeof(struct ip) + sizeof(struct icmphdr) + sizeof(struct ip) + sizeof(struct icmphdr)) {
				orig_icmp = (struct icmphdr *) (packet + sizeof(struct ip) + sizeof(struct icmphdr) + sizeof(struct ip));
				
				// no need to check the sub icmp header as it is include in the main checksum
				
				if (ntohs(orig_icmp->un.echo.id) != ping_lstnr->echo_id) {
					CRTX_DBG("different echo id: %d != %d\n", ntohs(orig_icmp->un.echo.id), ping_lstnr->echo_id);
				} else {
					CRTX_DBG("dest unreach %d %d\n", ntohs(orig_icmp->un.echo.sequence), icmp->code);
					send_event(ping_lstnr, CRTX_PING_ET_PING_UNREACHABLE, icmp, ssize - sizeof(struct ip));
				}
			} else {
				CRTX_ERROR("unexpected size of ICMP_DEST_UNREACH: %zd > %zu\n", ssize, sizeof(struct ip) + sizeof(struct icmphdr) + sizeof(struct ip));
				send_event(ping_lstnr, CRTX_PING_ET_PING_ERROR, icmp, ssize - sizeof(struct ip));
			}
		} else {
			CRTX_ERROR("unexpected icmp reply %d\n", icmp->type);
			int i;
			for (i=0;i<PACKET_LEN; i++)
				CRTX_ERROR("%02hhx ", packet[i]);
			CRTX_ERROR("\n");
		}
		
		return 0;
	}
	
	if (ntohs(icmp->un.echo.id) != ping_lstnr->echo_id) {
		CRTX_DBG("different echo id: %d != %d\n", ntohs(icmp->un.echo.id), ping_lstnr->echo_id);
		return 0;
	}
	CRTX_DBG("ping rx %d\n", ntohs(icmp->un.echo.sequence));
	
	send_event(ping_lstnr, CRTX_PING_ET_PING_SUCCESS, icmp, ssize - sizeof(struct ip));
	
	return 0;
}

static void fd_error_handler(struct crtx_evloop_callback *el_cb, void *userdata) {
	CRTX_ERROR("error from ping socket fd\n");
}

static char ping_start_listener(struct crtx_listener_base *listener) {
	struct crtx_ping_listener *ping_lstnr;
	int ret;
	
	ping_lstnr = (struct crtx_ping_listener*) listener;
	
	ret = crtx_start_listener(&ping_lstnr->timer_lstnr.base);
	if (ret) {
		CRTX_ERROR("starting timer listener failed\n");
		return ret;
	}
	
	return 0;
}

static char ping_stop_listener(struct crtx_listener_base *listener) {
	struct crtx_ping_listener *ping_lstnr;
	
	ping_lstnr = (struct crtx_ping_listener*) listener;
	
	crtx_stop_listener(&ping_lstnr->timer_lstnr.base);
	
	return 0;
}

static void ping_shutdown_listener(struct crtx_listener_base *listener) {
	struct crtx_ping_listener *ping_lstnr;
	
	ping_lstnr = (struct crtx_ping_listener*) listener;
	
	if (ping_lstnr->packet) {
		free(ping_lstnr->packet);
		ping_lstnr->packet = 0;
	}
	
	crtx_shutdown_listener(&ping_lstnr->timer_lstnr.base);
}

struct crtx_listener_base *crtx_setup_ping_listener(void *options) {
	struct crtx_ping_listener *ping_lstnr;
	int ret, sockfd;
	
	
	ping_lstnr = (struct crtx_ping_listener*) options;
	
	ping_lstnr->base.shutdown = &ping_shutdown_listener;
	ping_lstnr->base.start_listener = &ping_start_listener;
	ping_lstnr->base.stop_listener = &ping_stop_listener;
	
	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd == -1) {
		CRTX_ERROR("ping: creating socket failed: %s\n", strerror(errno));
		return 0;
	}
	
	ping_lstnr->dst.sin_family = AF_INET;
	
	ret = inet_aton(ping_lstnr->ip, &ping_lstnr->dst.sin_addr);
	if (ret == 0) {
		CRTX_ERROR("ping: could not parse \"%s\"\n", ping_lstnr->ip);
		return 0;
	}
	
	if (ping_lstnr->echo_id == 0)
		ping_lstnr->echo_id = getpid();
	
	crtx_evloop_init_listener(
		&ping_lstnr->base,
		sockfd,
		CRTX_EVLOOP_READ,
		0,
		&fd_event_handler,
		ping_lstnr,
		&fd_error_handler,
		ping_lstnr
		);
	
	{
		ret = crtx_setup_listener("timer", &ping_lstnr->timer_lstnr.base);
		if (ret) {
			CRTX_ERROR("create_listener(timer) failed: %s\n", strerror(-ret));
			return 0;
		}
		
		crtx_create_task(ping_lstnr->timer_lstnr.base.graph, 0, "ping_timer_handler", ping_timer_handler, ping_lstnr);
	}
	
	
	return &ping_lstnr->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(ping)

void crtx_ping_init() {}
void crtx_ping_finish() {}

#ifdef CRTX_TEST

static char ping_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_ping_listener *ping_lstnr;
	void *p;
	size_t size;
	struct icmphdr *icmp, *orig_icmp;
	struct crtx_dict *dict;
	
	ping_lstnr = (struct crtx_ping_listener*) userdata;
	
	crtx_event_get_data(event, &p, &size);
	
	icmp = (struct icmphdr*) p;
	
	orig_icmp = (struct icmphdr *) ((char*)icmp + sizeof(struct icmphdr) + sizeof(struct ip));
	
	if (event->type == CRTX_PING_ET_PING_SUCCESS) {
		dict = crtx_ping_raw2dict_addr(icmp, ICMP_T_ECHO);
		
		crtx_print_dict(dict);
		
		printf("ping %s success\n", ping_lstnr->ip);
	} else
	if (event->type == CRTX_PING_ET_PING_UNREACHABLE) {
		dict = crtx_ping_raw2dict_addr(orig_icmp, ICMP_T_ECHO);
		
		crtx_print_dict(dict);
		
		printf("ping %s unreachable code %d seq %d\n", ping_lstnr->ip, icmp->code, ntohs(orig_icmp->un.echo.sequence));
	} else
	if (event->type == CRTX_PING_ET_PING_ERROR) {
		printf("ping %s error\n", ping_lstnr->ip);
	}
	
	return 0;
}

int ping_main(int argc, char **argv) {
	struct crtx_ping_listener ping_lstnr;
	int r;
	
	
	memset(&ping_lstnr, 0, sizeof(struct crtx_ping_listener));
	
	ping_lstnr.ip = argv[1];
	
	// set time for (first) alarm
	ping_lstnr.timer_lstnr.newtimer.it_value.tv_sec = 1;
	ping_lstnr.timer_lstnr.newtimer.it_value.tv_nsec = 0;
	
	// set interval for repeating alarm, set to 0 to disable repetition
	ping_lstnr.timer_lstnr.newtimer.it_interval.tv_sec = 2;
	ping_lstnr.timer_lstnr.newtimer.it_interval.tv_nsec = 100e6;
	
	ping_lstnr.timer_lstnr.clockid = CLOCK_REALTIME; // clock source, see: man clock_gettime()
	ping_lstnr.timer_lstnr.settime_flags = 0; // absolute (TFD_TIMER_ABSTIME), or relative (0) time, see: man timerfd_settime()
	
	r = crtx_setup_listener("ping", &ping_lstnr);
	if (r) {
		CRTX_ERROR("create_listener(ping) failed\n");
		goto finish;
	}
	
	crtx_create_task(ping_lstnr.base.graph, 0, "ping_event_handler", ping_event_handler, &ping_lstnr);
	
	r = crtx_start_listener(&ping_lstnr.base);
	if (r) {
		CRTX_ERROR("starting ping listener failed\n");
		goto finish;
	}
	
	crtx_loop();

finish:
	crtx_shutdown_listener(&ping_lstnr.base);
	
	return 0;
}

CRTX_TEST_MAIN(ping_main);

#endif
