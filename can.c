
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

// #include <linux/if.h>
#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
// #include <netlink/route/rtnl.h>
#include <netlink/route/link.h>
#include <netlink/route/link/can.h>

// #include <libnetlink.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define NLMSG_TAIL(nmsg) \
	((struct rtattr *) (((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

static int addattr32(struct nlmsghdr *n, size_t maxlen, int type, __u32 data)
{
	int len = RTA_LENGTH(4);
	struct rtattr *rta;
	
	if (NLMSG_ALIGN(n->nlmsg_len) + len > maxlen) {
		fprintf(stderr,
				"addattr32: Error! max allowed bound %zu exceeded\n",
		  maxlen);
		return -1;
	}
	
	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), &data, 4);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + len;
	
	return 0;
}

static int addattr_l(struct nlmsghdr *n, size_t maxlen, int type,
					 const void *data, int alen)
{
	int len = RTA_LENGTH(alen);
	struct rtattr *rta;
	
	if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > maxlen) {
		fprintf(stderr,
				"addattr_l ERROR: message exceeded bound of %zu\n",
		  maxlen);
		return -1;
	}
	
	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);
	
	return 0;
}



#include "core.h"
#include "can.h"

// libnl-genl-3-dev libnl-genl-3-dev libnl-genl-3-dev

char *can_msg_etype[] = { CAN_MSG_ETYPE, 0 };

static char can_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *payload;
// 	struct crtx_udev_listener *ulist;
	struct can_frame frame;
	struct crtx_event *nevent;
	struct crtx_can_listener *clist;
	int r;
	
	payload = (struct crtx_event_loop_payload*) event->data.raw.pointer;
	
	clist = (struct crtx_can_listener *) payload->data;
	printf("read\n");
	r = read(clist->sockfd, &frame, sizeof(struct can_frame));
	if (r != sizeof(struct can_frame)) {
		fprintf(stderr, "wrong can frame size: %d\n", r);
		return 1;
	}
	
	printf("0x%04x ", frame.can_id);
	
	nevent = create_event("can", 0, 0);
	
	nevent->data.raw.type = 'U';
	memcpy(&nevent->data.raw.uint64, &frame.data, sizeof(uint64_t));
	
	add_event(clist->parent.graph, nevent);
	exit(1);
// 	dev = can_monitor_receive_device(clist->monitor);
// 	if (dev) {
// 		struct crtx_event *nevent;
// 		
// 		// size of struct can_device is unknown
// 		nevent = create_event(UDEV_MSG_ETYPE, dev, sizeof(struct can_device*));
// 		nevent->data.raw.flags |= DIF_DATA_UNALLOCATED;
// 		
// 		nevent->cb_before_release = &can_event_before_release_cb;
// 		// 		reference_event_release(nevent);
// 		
// 		add_event(clist->parent.graph, nevent);
// 		
// 		// 		wait_on_event(nevent);
// 		
// 		// 		dereference_event_release(nevent);
// 		
// 		// 		can_device_unref(dev);
// 	} else {
// 		DBG("can_monitor_receive_device returned no device\n");
// 	}
	
	return 0;
}

void crtx_free_can_listener(struct crtx_listener_base *data) {
// 	struct crtx_can_listener *clist;
// 	
// 	clist = (struct crtx_can_listener*) data;
	
	
}

void setup_if(int if_idx) {
	struct nl_sock *socket;
	struct nl_cache *cache;
	struct rtnl_link *link, *newlink;
	int r;
	
	socket = nl_socket_alloc();
	if (socket == 0){
		printf("Failed to allocate a netlink socket\n");
// 		ret = -1;
		goto error_socket;
	}
	
	r = nl_connect(socket, NETLINK_ROUTE);
	if (r != 0){
		printf("failed to connect to kernel\n");
// 		ret = -1;
		goto error_connect;
	}
	
	r = rtnl_link_alloc_cache(socket, AF_UNSPEC, &cache);
	if ( r < 0 ){
		printf("error creating link cache\n");
// 		ret = -1;
		goto error_connect;
	}
	
	//lookup interface
// 	struct rtnl_link *link = rtnl_link_get_by_name(cache, "can0");
	link = rtnl_link_get(cache, if_idx);
	if (link == 0){
		printf("No such interface %s\n", "ens33");
// 		ret = -1;
		goto error_cache;
	}
	
	if (!rtnl_link_is_can (link)) {
		printf("not a CAN interface\n");
		// 		ret = -1;
		goto error_cache;
	}
	
	char buf[64];
	rtnl_link_i2name (cache, if_idx, buf, 64);
	printf("name %s\n", buf);
	
// 	if (rtnl_link_get_kernel(socket, 0, "can1", &link) < 0)
// 		printf("error\n");
	
// 	unsigned char state = rtnl_link_get_operstate(link);
// 	switch (state) {
// 		case IF_OPER_UNKNOWN:
// 			printf("Unknown state\n");
// 			break;
// 		case IF_OPER_NOTPRESENT:
// 			printf("Link not present\n");
// 			break;
// 		case IF_OPER_DOWN:
// 			printf("Link down\n");
// 			break;
// 		case IF_OPER_LOWERLAYERDOWN:
// 			printf("L1 down\n");
// 			break;
// 		case IF_OPER_TESTING:
// 			printf("Testing\n");
// 			break;
// 		case IF_OPER_DORMANT:
// 			printf("Dormant\n");
// 			break;
// 		case IF_OPER_UP:
// 			printf("Link up\n");
// 			break;
// 	}
	
	unsigned int ret = rtnl_link_get_flags(link);
	switch(ret) {
		case IFF_UP: printf("up\n"); break;
		// 		case IFF_LOWER_UP: printf("IFF_LOWER_UP\n"); break;
		// 		case IFF_DORMANT: printf("IFF_DORMANT\n"); break;
		case IFF_RUNNING: printf("IFF_RUNNING\n"); break;
// 		case IFF_ECHO: printf("IFF_ECHO\n"); break;
// 		case IFF_NOARP: printf("IFF_NOARP\n"); break;
		
// 		default: printf("%u\n", ret);
	}
	
	uint32_t bit;
	rtnl_link_can_get_bitrate(link, &bit);
	printf("bit %u\n", bit);
	
	
	
	
	newlink = rtnl_link_alloc();
	if (!newlink)
		printf("err alloc\n");
	
	rtnl_link_set_type(newlink, "can");
	
	printf("newlink %p\n", newlink);
	
	r = rtnl_link_can_set_bitrate(newlink, 250000);
	if (r) {
		printf("error setting bitrate\n");
		goto error_cache;
	}
	rtnl_link_set_flags(newlink, IFF_UP);
	
	r = rtnl_link_change(socket, link, newlink, 0);
	if (r < 0) {
		printf("link change failed\n");
	}
	
// 	struct can_bittiming bt;
// 	bt.bitrate = 250000;
// 	ret = rtnl_link_can_set_bittiming(link, &bt);
// 	if (ret < 0)
// 		printf("bittiming error\n");
	
// 	rtnl_link_set_flags(link, IFF_UP);
// 	rtnl_link_set_operstate(link, IF_OPER_UP);
	
	printf("fini\n");
	
	rtnl_link_put(link);
	rtnl_link_put(newlink);
	
error_cache:
	nl_cache_free(cache);
	nl_close(socket);
	
error_connect:
	nl_socket_free(socket);
	
error_socket:
	return;
}















struct set_req {
	struct nlmsghdr n;
	struct ifinfomsg i;
	char buf[1024];
};

struct req_info {
	__u8 restart;
	__u8 disable_autorestart;
	__u32 restart_ms;
	struct can_ctrlmode *ctrlmode;
	struct can_bittiming *bittiming;
};


static int send_mod_request(int fd, struct nlmsghdr *n)
{
	int status;
	struct sockaddr_nl nladdr;
	struct nlmsghdr *h;
	
	struct iovec iov = {
		.iov_base = (void *)n,
		.iov_len = n->nlmsg_len
	};
	struct msghdr msg = {
		.msg_name = &nladdr,
		.msg_namelen = sizeof(nladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	char buf[16384];
	
	memset(&nladdr, 0, sizeof(nladdr));
	
	nladdr.nl_family = AF_NETLINK;
	nladdr.nl_pid = 0;
	nladdr.nl_groups = 0;
	
	n->nlmsg_seq = 0;
	n->nlmsg_flags |= NLM_F_ACK;
	
	status = sendmsg(fd, &msg, 0);
	
	if (status < 0) {
		perror("Cannot talk to rtnetlink");
		return -1;
	}
	
	iov.iov_base = buf;
	while (1) {
		iov.iov_len = sizeof(buf);
		status = recvmsg(fd, &msg, 0);
		for (h = (struct nlmsghdr *)buf; (size_t) status >= sizeof(*h);) {
			int len = h->nlmsg_len;
			int l = len - sizeof(*h);
			if (l < 0 || len > status) {
				if (msg.msg_flags & MSG_TRUNC) {
					fprintf(stderr, "Truncated message\n");
					return -1;
				}
				fprintf(stderr,
						"!!!malformed message: len=%d\n", len);
				return -1;
			}
			
			if (h->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err =
				(struct nlmsgerr *)NLMSG_DATA(h);
				if ((size_t) l < sizeof(struct nlmsgerr)) {
					fprintf(stderr, "ERROR truncated\n");
				} else {
					errno = -err->error;
					if (errno == 0)
						return 0;
					
					perror("RTNETLINK answers");
				}
				return -1;
			}
			status -= NLMSG_ALIGN(len);
			h = (struct nlmsghdr *)((char *)h + NLMSG_ALIGN(len));
		}
	}
	
	return 0;
}


static int open_nl_sock()
{
	int fd;
	int sndbuf = 32768;
	int rcvbuf = 32768;
	unsigned int addr_len;
	struct sockaddr_nl local;
	
	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		perror("Cannot open netlink socket");
		return -1;
	}
	
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&sndbuf, sizeof(sndbuf));
	
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&rcvbuf, sizeof(rcvbuf));
	
	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_groups = 0;
	
	if (bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("Cannot bind netlink socket");
		return -1;
	}
	
	addr_len = sizeof(local);
	if (getsockname(fd, (struct sockaddr *)&local, &addr_len) < 0) {
		perror("Cannot getsockname");
		return -1;
	}
	if (addr_len != sizeof(local)) {
		fprintf(stderr, "Wrong address length %d\n", addr_len);
		return -1;
	}
	if (local.nl_family != AF_NETLINK) {
		fprintf(stderr, "Wrong address family %d\n", local.nl_family);
		return -1;
	}
	return fd;
}

#define IF_DOWN 0
#define IF_UP 1

static int do_set_nl_link(int fd, __u8 if_state, const char *name,
						  struct req_info *req_info)
{
	struct set_req req;
	
	const char *type = "can";
	
	memset(&req, 0, sizeof(req));
	
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.n.nlmsg_type = RTM_NEWLINK;
	req.i.ifi_family = 0;
	
	req.i.ifi_index = if_nametoindex(name);
	if (req.i.ifi_index == 0) {
		fprintf(stderr, "Cannot find device \"%s\"\n", name);
		return -1;
	}
	
	if (if_state) {
		switch (if_state) {
			case IF_DOWN:
				req.i.ifi_change |= IFF_UP;
				req.i.ifi_flags &= ~IFF_UP;
				break;
			case IF_UP:
				req.i.ifi_change |= IFF_UP;
				req.i.ifi_flags |= IFF_UP;
				break;
			default:
				fprintf(stderr, "unknown state\n");
				return -1;
		}
	}
	
	if (req_info != NULL) {
		/* setup linkinfo section */
		struct rtattr *linkinfo = NLMSG_TAIL(&req.n);
		addattr_l(&req.n, sizeof(req), IFLA_LINKINFO, NULL, 0);
		addattr_l(&req.n, sizeof(req), IFLA_INFO_KIND, type,
				  strlen(type));
		/* setup data section */
		struct rtattr *data = NLMSG_TAIL(&req.n);
		addattr_l(&req.n, sizeof(req), IFLA_INFO_DATA, NULL, 0);
		
		if (req_info->restart_ms > 0 || req_info->disable_autorestart)
			addattr32(&req.n, 1024, IFLA_CAN_RESTART_MS,
					  req_info->restart_ms);
			
			if (req_info->restart)
				addattr32(&req.n, 1024, IFLA_CAN_RESTART, 1);
			
			if (req_info->bittiming != NULL) {
				addattr_l(&req.n, 1024, IFLA_CAN_BITTIMING,
						  req_info->bittiming,
			  sizeof(struct can_bittiming));
			}
			
			if (req_info->ctrlmode != NULL) {
				addattr_l(&req.n, 1024, IFLA_CAN_CTRLMODE,
						  req_info->ctrlmode,
			  sizeof(struct can_ctrlmode));
			}
			
			/* mark end of data section */
			data->rta_len = (void *)NLMSG_TAIL(&req.n) - (void *)data;
			
			/* mark end of link info section */
			linkinfo->rta_len =
			(void *)NLMSG_TAIL(&req.n) - (void *)linkinfo;
	}
	
	return send_mod_request(fd, &req.n);
}

static int set_link(const char *name, __u8 if_state, struct req_info *req_info)
{
	int fd;
	int err = 0;
	
	fd = open_nl_sock();
	if (fd < 0)
		goto err_out;
	
	err = do_set_nl_link(fd, if_state, name, req_info);
	if (err < 0)
		goto close_out;
	
	close_out:
	close(fd);
	err_out:
	return err;
}

int can_set_bittiming(const char *name, struct can_bittiming *bt)
{
	struct req_info req_info = {
		.bittiming = bt,
	};
	
	return set_link(name, IF_UP, &req_info);
}


int can_set_bitrate(const char *name, __u32 bitrate)
{
	struct can_bittiming bt;
	
	memset(&bt, 0, sizeof(bt));
	bt.bitrate = bitrate;
	
	return can_set_bittiming(name, &bt);
}













struct crtx_listener_base *crtx_new_can_listener(void *options) {
	struct crtx_can_listener *clist;
	int r;
	
	clist = (struct crtx_can_listener *) options;
	
	
	clist->sockfd = socket(PF_CAN, SOCK_RAW, clist->protocol);
	struct ifreq ifr;
	if (clist->interface_name) {
		
		
		if (strlen(clist->interface_name) >= IFNAMSIZ-1) {
			fprintf(stderr, "too long interface name\n");
			return 0;
		}
		
		strcpy(ifr.ifr_name, clist->interface_name);
		r = ioctl(clist->sockfd, SIOCGIFINDEX, &ifr);
		if (r != 0) {
			fprintf(stderr, "SIOCGIFINDEX failed on %s: %s\n", clist->interface_name, strerror(errno));
			return 0;
		}
		
		// not available anymore
// 		ifr.ifr_ifru.ifru_ivalue = 250000;
// 		r = ioctl(clist->sockfd, SIOCSCANBAUDRATE, &ifr);
// 		if (r != 0) {
// 			fprintf(stderr, "SIOCSCANBAUDRATE failed on %s: %s\n", clist->interface_name, strerror(errno));
// 			return 0;
// 		}
		
// 		r = ioctl(clist->sockfd, SIOCGIFFLAGS, &ifr);
// 		if (r != 0) {
// 			fprintf(stderr, "SIOCGIFFLAGS failed on %s: %s\n", clist->interface_name, strerror(errno));
// 			return 0;
// 		}
// 		ifr.ifr_flags |= IFF_UP;
// 		r = ioctl(clist->sockfd, SIOCSIFFLAGS, &ifr);
// 		if (r != 0) {
// 			fprintf(stderr, "SIOCSIFFLAGS failed on %s: %s\n", clist->interface_name, strerror(errno));
// 			return 0;
// 		}
		
		clist->addr.can_family = AF_CAN;
		clist->addr.can_ifindex = ifr.ifr_ifindex;
	}
	
	setup_if(clist->addr.can_ifindex);
// 	can_set_bitrate("can1", 250000);
// 	setup_if(clist->addr.can_ifindex);
	
// 	r = ioctl(clist->sockfd, SIOCGIFFLAGS, &ifr);
// 	if (r != 0) {
// 		fprintf(stderr, "SIOCGIFFLAGS failed on %s: %s\n", clist->interface_name, strerror(errno));
// 		return 0;
// 	}
// 	ifr.ifr_flags |= IFF_UP;
// 	r = ioctl(clist->sockfd, SIOCSIFFLAGS, &ifr);
// 	if (r != 0) {
// 		fprintf(stderr, "SIOCSIFFLAGS failed on %s: %s\n", clist->interface_name, strerror(errno));
// 		return 0;
// 	}
	
	r = bind(clist->sockfd, (struct sockaddr *)&clist->addr, sizeof(clist->addr));
	if (r != 0) {
		fprintf(stderr, "bind failed: %s\n", strerror(errno));
		return 0;
	}
	
// 	int flags, s;
// 	
// 	flags = fcntl (clist->sockfd, F_GETFL, 0);
// 	if (flags == -1)
// 	{
// 		perror ("fcntl");
// 		return -1;
// 	}
// 	
// 	flags |= O_NONBLOCK;
// 	s = fcntl (clist->sockfd, F_SETFL, flags);
// 	if (s == -1)
// 	{
// 		perror ("fcntl");
// 		return -1;
// 	}
// 
// 	#include <sys/socket.h>
// 	int       error = 0;
// 	socklen_t errlen = sizeof(error);
// 	if (getsockopt(el_payload->fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) == 0)
// 	{
// 		printf("error = %s\n", strerror(error));
// 	}

	
	printf("can fd %d\n", clist->sockfd);
	clist->parent.el_payload.fd = clist->sockfd;
	clist->parent.el_payload.data = clist;
	clist->parent.el_payload.event_handler = &can_fd_event_handler;
	clist->parent.el_payload.event_handler_name = "can fd handler";
	
// 	clist->parent.free = &crtx_free_can_listener;
	new_eventgraph(&clist->parent.graph, "can", can_msg_etype);
	
	return &clist->parent;
}

void crtx_can_init() {
}

void crtx_can_finish() {
}
