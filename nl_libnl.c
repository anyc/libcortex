
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "intern.h"
#include "core.h"
#include "nl_libnl.h"

// int crtx_libnl_msg2dict(struct nl_msg *msg, void *arg) {
// 	struct crtx_libnl_listener *libnl_lstnr;
// 	struct crtx_event *event;
// 	struct crtx_dict *dict;
// 	struct crtx_dict_item *di, *tdi;
// // 	struct crtx_genl_group *g;
// 	int i;
// 	struct sockaddr_nl *sock;
// 	struct nlmsghdr *nlh;
// 	
// 	
// 	libnl_lstnr = (struct crtx_libnl_listener*) arg;
// 	
// 	nlh = nlmsg_hdr(msg);
// 	sock = nlmsg_get_src(msg);
// 	
// 	
// 	
// 	
// 	struct nlmsghdr *nlh = nlmsg_hdr(msg);
// 	struct ifinfomsg *iface = NLMSG_DATA(nlh);
// 	struct rtattr *hdr = IFLA_RTA(iface);
// 	int remaining = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*iface));
// 	
// 	//printf("Got something.\n");
// 	//nl_msg_dump(msg, stdout);
// 	
// 	while (RTA_OK(hdr, remaining)) {
// 		//printf("Loop\n");
// 		
// 		if (hdr->rta_type == IFLA_IFNAME) {
// 			printf("Found network interface %d: %s\n", iface->ifi_index, (char *) RTA_DATA(hdr));
// 		}
// 		
// 		hdr = RTA_NEXT(hdr, remaining);
// 	}
// 	
// 	return NL_OK;
// 	
// 	
// 	
// 	
// // 	// find right crtx_genl_group from source socket group bitmap
// // 	// TODO this might not be reliable, use only one group per listener
// // 	g = libnl_lstnr->groups;
// // 	while (g->group && (((1 << (g->gid-1)) & sock->nl_groups) == 0)) {
// // 		g++;
// // 	}
// // 	
// // 	if (!g->group) {
// // 		fprintf(stderr, "did not find genl group for msg type %d\n", sock->nl_groups);
// // 		return 0;
// // 	}
// // 	
// // 	// validate msg and parse attributes
// // 	genlmsg_parse(nlh, 0, g->attrs, g->n_attrs, 0);
// // 	
// // 	event = crtx_create_event("netlink/genl");
// // 	dict = crtx_init_dict(0, 0, 0);
// // 	// 	dict = event->data.dict;
// // 	crtx_event_set_dict_data(event, dict, 0);
// // 	
// // 	tdi = crtx_get_first_item(g->attrs_template);
// // 	for (i=0; i < g->n_attrs; i++) {
// // 		if (g->attrs[i]) {
// // 			if (!g->parse_callbacks[i]) {
// // 				di = crtx_alloc_item(dict);
// // 				
// // 				switch (tdi->type) {
// // 					case 'u':
// // 						crtx_fill_data_item(di, 'u', tdi->key, nla_get_u32(g->attrs[i]), 0, 0);
// // 						break;
// // 					case 'p':
// // 						crtx_fill_data_item(di, 'p', tdi->key, nla_data(g->attrs[i]), tdi->size, CRTX_DIF_DONT_FREE_DATA);
// // 						break;
// // 					case 's':
// // 						// nla_strdup
// // 						crtx_fill_data_item(di, 's', tdi->key, nla_get_string(g->attrs[i]), 0, CRTX_DIF_DONT_FREE_DATA);
// // 						break;
// // 				}
// // 			} else {
// // 				g->parse_callbacks[i](g->attrs[i], dict);
// // 			}
// // 		}
// // 		if (tdi)
// // 			tdi = crtx_get_next_item(g->attrs_template, tdi);
// // 	}
// // 	
// // 	// 	crtx_print_dict(dict);
// // 	
// // 	crtx_add_event(libnl_lstnr->parent.graph, event);
// 	
// 	return 0;
// }

// handler that is called on file descriptor events
static char libnl_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *payload;
	struct crtx_libnl_listener *libnl_lstnr;
	int r;
	
	
	payload = (struct crtx_event_loop_payload*) event->data.pointer;
	libnl_lstnr = (struct crtx_libnl_listener *) payload->data;
	
	// nl_recvmsgs_default will eventually call the registered callback
	r = nl_recvmsgs_default(libnl_lstnr->sock);
	if (r) {
		fprintf(stderr, "nl_recvmsgs_default failed: %d\n", r);
	}
	
	return 0;
}

static char libnl_start_listener(struct crtx_listener_base *listener) {
	struct crtx_libnl_listener *libnl_lstnr;
	int ret;
	
	
	libnl_lstnr = (struct crtx_libnl_listener*) listener;
	
	ret = nl_connect(libnl_lstnr->sock, NETLINK_ROUTE); 
	if (ret < 0) {
		ERROR("nl_connect failed: %d\n", ret);
		return ret;
	}
	
	libnl_lstnr->parent.el_payload.fd = nl_socket_get_fd(libnl_lstnr->sock);
	
	return 0;
}

void free_libnl_listener(struct crtx_listener_base *lbase, void *userdata) {
	struct crtx_libnl_listener *libnl_lstnr;
	
	libnl_lstnr = (struct crtx_libnl_listener*) lbase;
	
	nl_socket_free(libnl_lstnr->sock);
}

struct crtx_listener_base *crtx_new_nl_libnl_listener(void *options) {
	struct crtx_libnl_listener *libnl_lstnr;
	struct crtx_libnl_callback *cb;
	int r;
	
	libnl_lstnr = (struct crtx_libnl_listener*) options;
	
	
	libnl_lstnr->sock = nl_socket_alloc();
	if (!libnl_lstnr->sock) {
		fprintf(stderr, "nl_socket_alloc failed\n");
		return 0;
	}
	
	nl_socket_disable_seq_check(libnl_lstnr->sock);
	
	cb = libnl_lstnr->callbacks;
	while (cb->func) {
		r = nl_socket_modify_cb(libnl_lstnr->sock, cb->type, cb->kind, cb->func, cb->arg);
		if (r) {
			fprintf(stderr, "nl_socket_modify_cb failed %d\n", r);
			nl_socket_free(libnl_lstnr->sock);
			return 0;
		}
		cb++;
	}
	
	libnl_lstnr->parent.start_listener = &libnl_start_listener;
	
	libnl_lstnr->parent.el_payload.data = libnl_lstnr;
	libnl_lstnr->parent.el_payload.event_flags = EPOLLIN;
	libnl_lstnr->parent.el_payload.event_handler = &libnl_fd_event_handler;
	libnl_lstnr->parent.el_payload.event_handler_name = "libnl fd handler";
	libnl_lstnr->parent.free = &free_libnl_listener;
	
	
	
	return &libnl_lstnr->parent;
}

void crtx_nl_libnl_init() {
}

void crtx_nl_libnl_finish() {
}





#ifdef CRTX_TEST

#include <linux/netlink.h>
#include <netlink/genl/genl.h> // for struct nlmsghdr

#include "nl_route_raw.h"

static char libnl_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_libnl_listener *libnl;
	int r;
	
	libnl = (struct crtx_libnl_listener *) event->origin;
	
	if (event->data.type == 'u' && event->data.uint32 == CRTX_LSTNR_STARTED) {
		struct rtgenmsg rt_hdr = { .rtgen_family = AF_PACKET, };
		
		r = nl_send_simple(libnl->sock, RTM_GETLINK, NLM_F_REQUEST | NLM_F_DUMP, &rt_hdr, sizeof(rt_hdr));
		if (r < 0) {
			ERROR("nl_send_simple failed: %d\n", r);
			return 1;
		}
	}
	
	return 0;
}

int crtx_libnl_msg2dict(struct nl_msg *msg, void *arg) {
// 	struct crtx_libnl_listener *libnl_lstnr;
// 	struct nlmsghdr *nlh;
// 	struct ifinfomsg *iface;
// 	struct rtattr *hdr;
// 	int remaining;
	
	
// 	libnl_lstnr = (struct crtx_libnl_listener*) arg;
	
// 	nlh = nlmsg_hdr(msg);
// 	iface = NLMSG_DATA(nlh);
// 	hdr = IFLA_RTA(iface);
// 	remaining = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*iface));
// 	while (RTA_OK(hdr, remaining)) {
// 		if (hdr->rta_type == IFLA_IFNAME) {
// 			printf("Found network interface %d: %s\n", iface->ifi_index, (char *) RTA_DATA(hdr));
// 		}
// 		
// 		hdr = RTA_NEXT(hdr, remaining);
// 	}
	
	struct crtx_dict *dict;
	
	dict = crtx_nl_route_raw2dict_interface(nlmsg_hdr(msg), 1);
	
	crtx_print_dict(dict);
	
	return NL_OK;
}

int crtx_libnl_msg_end(struct nl_msg *msg, void *arg) {
	crtx_init_shutdown();
	
	return NL_OK;
}

struct crtx_libnl_callback libnl_callbacks[] = {
	{ NL_CB_VALID, NL_CB_CUSTOM, crtx_libnl_msg2dict, 0},
	{ NL_CB_FINISH, NL_CB_CUSTOM, crtx_libnl_msg_end, 0},
	{ 0 },
};

int libnl_main(int argc, char **argv) {
	struct crtx_libnl_listener libnl;
	struct crtx_listener_base *lbase;
	int r;
	
	
	memset(&libnl, 0, sizeof(struct crtx_libnl_listener));
	
	libnl_callbacks[0].arg = &libnl;
	
	libnl.callbacks = libnl_callbacks;
	
	lbase = create_listener("nl_libnl", &libnl);
	if (!lbase) {
		ERROR("create_listener(libnl) failed\n");
		exit(1);
	}
	
	lbase->state_graph = lbase->graph;
	crtx_create_task(lbase->graph, 0, "libnl_test", libnl_test_handler, 0);
	
	r = crtx_start_listener(lbase);
	if (!r) {
		ERROR("starting libnl listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(libnl_main);
#endif
