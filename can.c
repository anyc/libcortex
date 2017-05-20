
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <cap-ng.h>

#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/link/can.h>

#include "core.h"
#include "can.h"

// libnl-genl-3-dev libnl-genl-3-dev libnl-genl-3-dev

// char *can_msg_etype[] = { CAN_MSG_ETYPE, 0 };

// void can_event_before_release_cb(struct crtx_event *event) {
// 	free(event->data.pointer);
// }

#ifndef CRTX_TEST

static char can_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *payload;
	struct can_frame *frame;
	struct crtx_event *nevent;
	struct crtx_can_listener *clist;
	int r;
	
	payload = (struct crtx_event_loop_payload*) event->data.pointer;
	
	clist = (struct crtx_can_listener *) payload->data;
	
	frame = (struct can_frame *) malloc(sizeof(struct can_frame));
	r = read(clist->sockfd, frame, sizeof(struct can_frame));
	if (r != sizeof(struct can_frame)) {
		fprintf(stderr, "wrong can frame size: %d\n", r);
		return 1;
	}
	
// 	printf("0x%04x ", frame->can_id);
	
	nevent = crtx_create_event("can"); //, frame, sizeof(frame));
	crtx_event_set_raw_data(nevent, 'p', frame, sizeof(frame), 0);
// 	nevent->cb_before_release = &can_event_before_release_cb;
	
// 	memcpy(&nevent->data.uint64, &frame.data, sizeof(uint64_t));
	
	crtx_add_event(clist->parent.graph, nevent);
	
// 	exit(1);
	
	return 0;
}

void crtx_shutdown_can_listener(struct crtx_listener_base *data) {
	struct crtx_can_listener *clist;
	
	clist = (struct crtx_can_listener*) data;
	
	close(clist->sockfd);
// 	shutdown(inlist->server_sockfd, SHUT_RDWR);
}

static char *print_can_state (uint32_t state)
{
	char *text;
	
	switch (state)
	{
		case CAN_STATE_ERROR_ACTIVE:
			text = "error active";
			break;
		case CAN_STATE_ERROR_WARNING:
			text = "error warning";
			break;
		case CAN_STATE_ERROR_PASSIVE:
			text = "error passive";
			break;
		case CAN_STATE_BUS_OFF:
			text = "bus off";
			break;
		case CAN_STATE_STOPPED:
			text = "stopped";
			break;
		case CAN_STATE_SLEEPING:
			text = "sleeping";
			break;
		default:
			text = "unknown state";
	}
	
	printf("can state: %s\n", text);
	return text;
}

static int setup_can_if(struct crtx_can_listener *clist, char *if_name) {
	struct rtnl_link *newlink;
	int r;
	unsigned int flags;
	uint32_t bitrate;
	
	r = 0;
	
	if (!clist->socket) {
		clist->socket = nl_socket_alloc();
		if (clist->socket == 0){
			printf("Failed to allocate a netlink socket\n");
			r = -1;
			goto error_socket;
		}
		
		r = nl_connect(clist->socket, NETLINK_ROUTE);
		if (r != 0){
			printf("failed to connect to kernel\n");
			goto error_connect;
		}
	}
	
	if (!clist->link) {
		r = rtnl_link_get_kernel(clist->socket, 0, if_name, &clist->link);
		if (r != 0){
			printf("failed find interface\n");
			goto error_connect;
		}
		
		r = rtnl_link_is_can(clist->link);
		if (!r) {
			printf("not a CAN interface\n");
			r = -1;
			goto error_linktype;
		}
	}
	
	
	bitrate = 0;
	flags = rtnl_link_get_flags(clist->link);
	rtnl_link_can_get_bitrate(clist->link, &bitrate);
	
	if ( !(flags & IFF_UP) || (bitrate != clist->bitrate) ) {
// 		if (getuid() != 0 && geteuid() != 0) {
// 			printf("warning, interface \"%s\" need to be reconfigured, you might not have the required privileges.\n", if_name);
// 		}
		
		newlink = rtnl_link_alloc();
		if (!newlink) {
			printf("error allocating link\n");
			goto error_linktype;
		}
		
		int caps;
		caps = capng_get_caps_process();
		if (caps != 0) {
			ERROR("retrieving capabilities failed\n");
		} else {
			if (!capng_have_capability(CAPNG_EFFECTIVE, CAP_NET_ADMIN)) {
				char *buf;
				
				ERROR("interface \"%s\" needs to be reconfigured and it looks like you do not have the required privileges (CAP_NET_ADMIN).\n", if_name);
				buf = capng_print_caps_text(CAPNG_PRINT_BUFFER, CAPNG_EFFECTIVE);
				DBG("effective caps: %s\n", buf);
				free(buf);
			}
		}
		
		rtnl_link_set_type(newlink, "can");
		
		if (bitrate != clist->bitrate) {
			r = rtnl_link_can_set_bitrate(newlink, clist->bitrate);
			if (r) {
				printf("error setting bitrate\n");
				goto error_newlink;
			}
		}
		
		if (!(flags & IFF_UP))
			rtnl_link_set_flags(newlink, IFF_UP);
		
		r = rtnl_link_change(clist->socket, clist->link, newlink, 0);
		if (r < 0) {
			printf("link change failed: %s\n", nl_geterror(r));
			goto error_newlink;
		}
		
		return 1;
		
		error_newlink:
			rtnl_link_put(newlink);
	}
	
// 	printf("bit %u\n", bitrate);
	
// 	if (bitrate != clist->bitrate && (flags & IFF_UP)) {
// 		rtnl_link_unset_flags(newlink, IFF_UP);
// 		
// 		r = rtnl_link_change(socket, link, newlink, 0);
// 		if (r < 0) {
// 			printf("link change failed: %s\n", nl_geterror(r));
// 		}
// 	}
	

	
	return r;
	
error_linktype:
	rtnl_link_put(clist->link);
	clist->link = 0;
	
error_connect:
	nl_close(clist->socket);
	
error_socket:
	nl_socket_free(clist->socket);
	clist->socket = 0;
	
	return r;
}

static void on_error_cb(struct crtx_event_loop_payload *el_payload, void *data) {
	struct crtx_can_listener *clist;
	
	clist = (struct crtx_can_listener *) data;
	
	crtx_stop_listener(&clist->parent);
}

struct crtx_listener_base *crtx_new_can_listener(void *options) {
	struct crtx_can_listener *clist;
	int r;
	
	
	clist = (struct crtx_can_listener *) options;
	
	clist->sockfd = socket(PF_CAN, SOCK_RAW, clist->protocol);
	if (clist->sockfd == -1) {
		ERROR("opening can socket failed: %s\n", strerror(errno));
		return 0;
	}
	
	if (clist->interface_name) {
		struct ifreq ifr;
		
		if (strlen(clist->interface_name) >= IFNAMSIZ-1) {
			fprintf(stderr, "too long interface name\n");
			close(clist->sockfd);
			return 0;
		}
		
		strcpy(ifr.ifr_name, clist->interface_name);
		r = ioctl(clist->sockfd, SIOCGIFINDEX, &ifr);
		if (r != 0) {
			fprintf(stderr, "SIOCGIFINDEX failed on %s (%d): %s\n", clist->interface_name, clist->sockfd, strerror(errno));
			close(clist->sockfd);
			return 0;
		}
		
		clist->addr.can_family = AF_CAN;
		clist->addr.can_ifindex = ifr.ifr_ifindex;
		
		r = setup_can_if(clist, clist->interface_name);
		if (r < 0) {
			close(clist->sockfd);
			return 0;
		}
// 		// if we send a configuration command to the kernel, we do not start
// 		// listening now and wait for the "interface is up" message
// 		if (r == 1) {
// 			close(clist->sockfd);
// 			return 0;
// 		}
		
		uint32_t can_state;
		r = rtnl_link_can_state (clist->link, &can_state);
		if (r < 0) {
			printf("link change failed: %s\n", nl_geterror(r));
		}
		
		print_can_state(can_state);
	}
	
	
	r = bind(clist->sockfd, (struct sockaddr *)&clist->addr, sizeof(clist->addr));
	if (r != 0) {
		fprintf(stderr, "bind failed: %s\n", strerror(errno));
		close(clist->sockfd);
		return 0;
	}
	
	clist->parent.el_payload.fd = clist->sockfd;
	clist->parent.el_payload.data = clist;
	clist->parent.el_payload.event_handler = &can_fd_event_handler;
	clist->parent.el_payload.event_handler_name = "can fd handler";
	clist->parent.el_payload.error_cb = &on_error_cb;
	clist->parent.el_payload.error_cb_data = clist;
	
	clist->parent.shutdown = &crtx_shutdown_can_listener;
	
// 	new_eventgraph(&clist->parent.graph, "can", can_msg_etype);
	
	return &clist->parent;
}

void crtx_can_init() {
}

void crtx_can_finish() {
}

#else /* CRTX_TEST */

static char can_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct can_frame *frame;
	
	frame = (struct can_frame*) event->data.pointer;
	
	printf("CAN: %d\n", frame->can_id);
	
	return 0;
}

int can_main(int argc, char **argv) {
	struct crtx_can_listener clist;
	struct crtx_listener_base *lbase;
	int ret;
	
	
	memset(&clist, 0, sizeof(struct crtx_can_listener));
	clist.protocol = CAN_RAW;
	clist.bitrate = 250000;
	
	if (argc > 1)
		clist.interface_name = argv[1];
	
	lbase = create_listener("can", &clist);
	if (!lbase) {
		ERROR("create_listener(can) failed\n");
		return 0;
	}
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting can listener failed\n");
		return 0;
	}
	
	
	crtx_create_task(lbase->graph, 0, "can_test", &can_event_handler, 0);
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(can_main);
#endif
