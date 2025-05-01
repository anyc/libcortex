/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#ifndef CRTX_CAN_NO_IF_SETUP
#include <cap-ng.h>
#endif

#ifndef CRTX_CAN_NO_IF_SETUP
#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/route/link.h>
#include <netlink/route/link/can.h>
#endif

#include <linux/can/raw.h>

#include "intern.h"
#include "core.h"
#include "can.h"

// libnl-genl-3-dev libnl-genl-3-dev libnl-genl-3-dev

// char *can_msg_etype[] = { CAN_MSG_ETYPE, 0 };

// void can_event_before_release_cb(struct crtx_event *event) {
// 	free(event->data.pointer);
// }

#ifndef CRTX_TEST

static char can_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct can_frame *frame;
	struct crtx_event *nevent;
	struct crtx_can_listener *clist;
	ssize_t r;
	
	int c=-1;
	clist = (struct crtx_can_listener *) userdata;
	
	// only create event if there are tasks in the queue
	if (clist->base.graph->tasks) {
		while (1) {
			frame = (struct can_frame *) malloc(sizeof(struct can_frame));
			c+=1;
			r = read(clist->sockfd, frame, sizeof(struct can_frame));
			if (r < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					free(frame);
					break;
				} else {
					CRTX_ERROR("read can frame failed: %s\n", strerror(errno));
					free(frame);
					return 1;
				}
			} else
			if (r != sizeof(struct can_frame)) {
				CRTX_ERROR("wrong can frame size: %zd\n", r);
				free(frame);
				return 1;
			}
			
			r = crtx_create_event(&nevent);
			if (r) {
				CRTX_ERROR("crtx_create_event failed: %s\n", strerror(r));
				free(frame);
			} else {
				nevent->description = "can";
				crtx_event_set_raw_data(nevent, 'p', frame, sizeof(frame), 0);
				
				crtx_add_event(clist->base.graph, nevent);
			}
			
			if (!clist->nonblocking) {
				break;
			}
		}
	} else {
		if (clist->discard_unused_data) {
			while (1) {
				// consume and discard the data
				struct can_frame static_frame;
				r = read(clist->sockfd, &static_frame, sizeof(struct can_frame));
				if (r != sizeof(struct can_frame)) {
					break;
				}
			}
		}
	}
	
	return 0;
}

static void shutdown_listener(struct crtx_listener_base *data) {
	struct crtx_can_listener *clist;
	
	clist = (struct crtx_can_listener*) data;
	
	#ifndef CRTX_CAN_NO_IF_SETUP
	rtnl_link_put(clist->link);
	nl_close(clist->socket);
	nl_socket_free(clist->socket);
	clist->socket = 0;
	#endif
	
	if (clist->sockfd >= 0)
		close(clist->sockfd);
	
// 	shutdown(inlist->server_sockfd, SHUT_RDWR);
}

#ifndef CRTX_CAN_NO_IF_SETUP
static char *print_can_state (uint32_t state)
{
	char *text;
	
	
// 	CAN_STATE_ERROR_ACTIVE = 0,	/* RX/TX error count < 96 */
// 	CAN_STATE_ERROR_WARNING,	/* RX/TX error count < 128 */
// 	CAN_STATE_ERROR_PASSIVE,	/* RX/TX error count < 256 */
// 	CAN_STATE_BUS_OFF,		/* RX/TX error count >= 256 */
// 	CAN_STATE_STOPPED,		/* Device is stopped */
// 	CAN_STATE_SLEEPING,		/* Device is sleeping */
	
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
#endif

#ifndef CRTX_CAN_NO_IF_SETUP
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
		
		// rtnl_link_is_can() does not return true for vcan interfaces
		/*r = rtnl_link_is_can(clist->link);
		if (!r) {
			printf("not a CAN interface\n");
			r = -1;
			goto error_linktype;
		}*/
		
		char *type = rtnl_link_get_type(clist->link);
		if (strcmp(type, "can") && strcmp(type, "vcan")) {
			r = -1;
			goto error_linktype;
		}

		if (!strcmp(type, "vcan"))
			clist->is_virtual_can = 1;
	}
	
	
	bitrate = 0;
	flags = rtnl_link_get_flags(clist->link);

	if (!clist->is_virtual_can) {
		rtnl_link_can_get_bitrate(clist->link, &bitrate);
		
		CRTX_DBG("current bitrate: %u\n", bitrate);
	} else {
		bitrate = clist->bitrate;
	}
	
	if (clist->ignore_if_state) {
		clist->bitrate = bitrate;
	} else
	if (clist->try_to_keep_if_up) {
		if ( !(flags & IFF_UP) || (bitrate != clist->bitrate) ) {
			int caps;
			caps = capng_get_caps_process();
			if (caps != 0) {
				CRTX_ERROR("retrieving capabilities failed\n");
			} else {
				if (!capng_have_capability(CAPNG_EFFECTIVE, CAP_NET_ADMIN)) {
					char *buf;
					
					CRTX_ERROR("interface \"%s\" needs to be reconfigured and it looks like you do not have the required privileges (CAP_NET_ADMIN).\n", if_name);
					buf = capng_print_caps_text(CAPNG_PRINT_BUFFER, CAPNG_EFFECTIVE);
					CRTX_DBG("effective caps: %s\n", buf);
					free(buf);
				}
			}
			
			newlink = rtnl_link_alloc();
			if (!newlink) {
				printf("error allocating link\n");
				goto error_linktype;
			}
			
			rtnl_link_set_type(newlink, "can");
			
			if (bitrate != clist->bitrate) {
				CRTX_DBG("will change bitrate from %u to %u\n", bitrate, clist->bitrate);
				
				r = rtnl_link_can_set_bitrate(newlink, clist->bitrate);
				if (r) {
					printf("error setting bitrate\n");
					goto error_newlink1;
				}
			}
			
			if (!(flags & IFF_UP))
				rtnl_link_set_flags(newlink, IFF_UP);
			
			r = rtnl_link_change(clist->socket, clist->link, newlink, 0);
			if (r < 0) {
				printf("link change failed: %s\n", nl_geterror(r));
				goto error_newlink1;
			}
			
			r = 1;
			
		error_newlink1:
			rtnl_link_put(newlink);
		}
		
	} else {
		int caps;
		caps = capng_get_caps_process();
		if (caps != 0) {
			CRTX_ERROR("retrieving capabilities failed\n");
		} else {
			if (!capng_have_capability(CAPNG_EFFECTIVE, CAP_NET_ADMIN)) {
				char *buf;
				
				CRTX_ERROR("interface \"%s\" needs to be reconfigured and it looks like you do not have the required privileges (CAP_NET_ADMIN).\n", if_name);
				buf = capng_print_caps_text(CAPNG_PRINT_BUFFER, CAPNG_EFFECTIVE);
				CRTX_DBG("effective caps: %s\n", buf);
				free(buf);
			}
		}
		
		if (flags & IFF_UP) {
			newlink = rtnl_link_alloc();
			if (!newlink) {
				printf("error allocating link\n");
				goto error_linktype;
			}
			
			rtnl_link_set_type(newlink, "can");
			
			rtnl_link_unset_flags(newlink, IFF_UP);
			
			CRTX_DBG("restarting interface\n");
			
			r = rtnl_link_change(clist->socket, clist->link, newlink, 0);
			if (r < 0) {
				printf("link change failed: %s\n", nl_geterror(r));
				goto error_newlink;
			}
			
			rtnl_link_put(newlink);
		}
		
		newlink = rtnl_link_alloc();
		if (!newlink) {
			printf("error allocating link\n");
			goto error_linktype;
		}
		
		rtnl_link_set_type(newlink, "can");
		
		if (bitrate != clist->bitrate) {
			CRTX_DBG("will change bitrate from %u to %u\n", bitrate, clist->bitrate);
			
			r = rtnl_link_can_set_bitrate(newlink, clist->bitrate);
			if (r) {
				printf("error setting bitrate\n");
				goto error_newlink;
			}
		}
		
		if (0) {
			unsigned int new_txqlen, txqlen;
			
			new_txqlen = 1000;
			txqlen = rtnl_link_get_txqlen(clist->link);
			if (txqlen != new_txqlen) {
				rtnl_link_set_txqlen(newlink, new_txqlen);
				
				CRTX_DBG("changing tx queue length from %u to %u\n", txqlen, new_txqlen);
			}
		}
		
		rtnl_link_set_flags(newlink, IFF_UP);
		
		r = rtnl_link_change(clist->socket, clist->link, newlink, 0);
		if (r < 0) {
			printf("link change failed: %s\n", nl_geterror(r));
			goto error_newlink;
		}
		
		r = 1;
		
	error_newlink:
		rtnl_link_put(newlink);
	}
	
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
#endif

static void on_error_cb(struct crtx_evloop_callback *el_cb, void *data) {
	struct crtx_can_listener *clist;
	
	clist = (struct crtx_can_listener *) data;
	
	crtx_stop_listener(&clist->base);
}

static char start_listener(struct crtx_listener_base *lstnr) {
	struct crtx_can_listener *clist;
	int r;
	
	
	clist = (struct crtx_can_listener*) lstnr;
	
	if (clist->sockfd >= 0)
		return 0;
	
	clist->sockfd = socket(PF_CAN, SOCK_RAW, clist->protocol);
	if (clist->sockfd == -1) {
		CRTX_ERROR("opening can socket failed: %s\n", strerror(errno));
		return 0;
	}
	
	if (clist->send_buffer_size > 0 || clist->recv_buffer_size > 0) {
		int sendBufSize, recvBufSize;
		socklen_t intSize;
		
		
		intSize = sizeof(int);
		
		if (clist->recv_buffer_size > 0) {
			r = getsockopt(clist->sockfd, SOL_SOCKET, SO_RCVBUF, &recvBufSize, &intSize);
			if (r != 0) {
				CRTX_ERROR("unable to read receive buffer size\n");
			}
			
			r = setsockopt(clist->sockfd, SOL_SOCKET, SO_RCVBUF,  &clist->recv_buffer_size, sizeof(clist->recv_buffer_size));
			if (r != 0) {
				CRTX_ERROR("unable to change receive buffer size\n");
			}
			
			r = getsockopt(clist->sockfd, SOL_SOCKET, SO_RCVBUF, &clist->recv_buffer_size, &intSize);
			if (r != 0) {
				CRTX_ERROR("unable to read receive buffer size\n");
			}
			
			printf("changing receive buffer from %d to %d\n", recvBufSize, clist->recv_buffer_size);
		}
		
		if (clist->send_buffer_size > 0) {
			r = getsockopt(clist->sockfd, SOL_SOCKET, SO_SNDBUF, &sendBufSize, &intSize);
			if (r != 0) {
				CRTX_ERROR("unable to read send buffer size\n");
			}
			
			r = setsockopt(clist->sockfd, SOL_SOCKET, SO_SNDBUF,  &clist->send_buffer_size, sizeof(clist->send_buffer_size));
			if (r != 0) {
				CRTX_ERROR("unable to change send buffer size\n");
			}
			
			r = getsockopt(clist->sockfd, SOL_SOCKET, SO_SNDBUF, &clist->send_buffer_size, &intSize);
			if (r != 0) {
				CRTX_ERROR("unable to read send buffer size\n");
			}
			
			printf("changing send buffer from %d to %d\n", sendBufSize, clist->send_buffer_size);
		}
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
		
		#ifndef CRTX_CAN_NO_IF_SETUP
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
		
		if (!clist->is_virtual_can) {
			uint32_t can_state;
			r = rtnl_link_can_state(clist->link, &can_state);
			if (r < 0) {
				printf("link change failed: %s\n", nl_geterror(r));
			}
			
			print_can_state(can_state);
		}
		#endif
	}
	
	/* TODO cangen does this to remove the default filter (?) from the kernel */
// 	setsockopt(clist->sockfd, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);
	
// 	int loopback = 0;
// 	setsockopt(clist->sockfd, SOL_CAN_RAW, CAN_RAW_LOOPBACK,
// 			   &loopback, sizeof(loopback));

	if (!clist->setup_if_only) {
		r = bind(clist->sockfd, (struct sockaddr *)&clist->addr, sizeof(clist->addr));
		if (r != 0) {
			fprintf(stderr, "bind failed: %s\n", strerror(errno));
			close(clist->sockfd);
			return 0;
		}
		
		crtx_evloop_init_listener(&clist->base,
							&clist->sockfd, 0,
							CRTX_EVLOOP_READ,
							0,
							&can_fd_event_handler,
							clist,
							&on_error_cb, clist
		);
	}
	
	return 0;
}

struct crtx_listener_base *crtx_setup_can_listener(void *options) {
	struct crtx_can_listener *clist;
	
	
	clist = (struct crtx_can_listener *) options;
	
	clist->base.start_listener = &start_listener;
	clist->sockfd = -1;
	
	if (!clist->setup_if_only) {
		clist->base.shutdown = &shutdown_listener;
	} else {
		// we have no fd or thread to monitor
		clist->base.mode = CRTX_NO_PROCESSING_MODE;
	}
	
	return &clist->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(can)

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
	int ret;
	
	
	memset(&clist, 0, sizeof(struct crtx_can_listener));
	clist.protocol = CAN_RAW;
	clist.bitrate = 250000;
	
	if (argc > 1)
		clist.interface_name = argv[1];
	
	ret = crtx_setup_listener("can", &clist);
	if (ret) {
		CRTX_ERROR("create_listener(can) failed: %s\n", strerror(-ret));
		return 0;
	}
	
	ret = crtx_start_listener(&clist.base);
	if (ret) {
		CRTX_ERROR("starting can listener failed\n");
		return 0;
	}
	
	
	crtx_create_task(clist.base.graph, 0, "can_test", &can_event_handler, 0);
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(can_main);
#endif
