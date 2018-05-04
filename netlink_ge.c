/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include "intern.h"
#include "core.h"
#include "netlink_ge.h"

// parse nl_msg and create a crtx_dict
int crtx_genl_msg2dict(struct nl_msg *msg, void *arg) {
	struct crtx_genl_listener *genlist;
	struct crtx_event *event;
	struct crtx_dict *dict;
	struct crtx_dict_item *di, *tdi;
	struct crtx_genl_group *g;
	int i;
	struct sockaddr_nl *sock;
	struct nlmsghdr *nlh;
	
	
	genlist = (struct crtx_genl_listener*) arg;
	
	nlh = nlmsg_hdr(msg);
	sock = nlmsg_get_src(msg);
	
	// find right crtx_genl_group from source socket group bitmap
	// TODO this might not be reliable, use only one group per listener
	g = genlist->groups;
	while (g->group && (((1 << (g->gid-1)) & sock->nl_groups) == 0)) {
		g++;
	}
	
	if (!g->group) {
		fprintf(stderr, "did not find genl group for msg type %d\n", sock->nl_groups);
		return 0;
	}
	
	// validate msg and parse attributes
	genlmsg_parse(nlh, 0, g->attrs, g->n_attrs, 0);
	
	event = crtx_create_event("netlink/genl");
	dict = crtx_init_dict(0, 0, 0);
// 	dict = event->data.dict;
	crtx_event_set_dict_data(event, dict, 0);
	
	tdi = crtx_get_first_item(g->attrs_template);
	for (i=0; i < g->n_attrs; i++) {
		if (g->attrs[i]) {
			if (!g->parse_callbacks[i]) {
				di = crtx_alloc_item(dict);
				
				switch (tdi->type) {
					case 'u':
						crtx_fill_data_item(di, 'u', tdi->key, nla_get_u32(g->attrs[i]), 0, 0);
						break;
					case 'p':
						crtx_fill_data_item(di, 'p', tdi->key, nla_data(g->attrs[i]), tdi->size, CRTX_DIF_DONT_FREE_DATA);
						break;
					case 's':
						// nla_strdup
						crtx_fill_data_item(di, 's', tdi->key, nla_get_string(g->attrs[i]), 0, CRTX_DIF_DONT_FREE_DATA);
						break;
				}
			} else {
				g->parse_callbacks[i](g->attrs[i], dict);
			}
		}
		if (tdi)
			tdi = crtx_get_next_item(g->attrs_template, tdi);
	}
	
// 	crtx_print_dict(dict);
	
	crtx_add_event(genlist->parent.graph, event);
	
	return 0;
}

// handler that is called on file descriptor events
static char genl_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *payload;
	struct crtx_genl_listener *genlist;
	int r;
	
	
	payload = (struct crtx_event_loop_payload*) event->data.pointer;
	genlist = (struct crtx_genl_listener *) payload->data;
	
	// nl_recvmsgs_default will eventually call the registered callback
	r = nl_recvmsgs_default(genlist->sock);
	if (r) {
		fprintf(stderr, "nl_recvmsgs_default failed: %d\n", r);
	}
	
	return 0;
}

void free_genl_listener(struct crtx_listener_base *lbase, void *userdata) {
	struct crtx_genl_listener *genlist;
	
	genlist = (struct crtx_genl_listener*) lbase;
	
	nl_socket_free(genlist->sock);
}

struct crtx_listener_base *crtx_new_genl_listener(void *options) {
	struct crtx_genl_listener *genlist;
	struct crtx_genl_group *g;
	struct crtx_genl_callback *cb;
	int r;
	
	genlist = (struct crtx_genl_listener*) options;
	
	
	genlist->sock = nl_socket_alloc();
	if (!genlist->sock) {
		fprintf(stderr, "nl_socket_alloc failed\n");
		return 0;
	}
	
	nl_socket_disable_seq_check(genlist->sock);
	
	cb = genlist->callbacks;
	while (cb->func) {
		r = nl_socket_modify_cb(genlist->sock, cb->type, cb->kind, cb->func, cb->arg);
		if (r) {
			fprintf(stderr, "nl_socket_modify_cb failed %d\n", r);
			nl_socket_free(genlist->sock);
			return 0;
		}
		cb++;
	}
	
	r = genl_connect(genlist->sock);
	if (r) {
		fprintf(stderr, "genl_connect failed: %d\n", r);
		nl_socket_free(genlist->sock);
		return 0;
	}
	
	
// 	new_eventgraph(&genlist->parent.graph, 0, 0);
	
	genlist->parent.el_payload.fd = nl_socket_get_fd(genlist->sock);
	genlist->parent.el_payload.data = genlist;
	genlist->parent.el_payload.event_handler = &genl_fd_event_handler;
	genlist->parent.el_payload.event_handler_name = "genl fd handler";
	genlist->parent.free = &free_genl_listener;
	
	
	g = genlist->groups;
	while (g->group) {
		g->gid = genl_ctrl_resolve_grp(genlist->sock, g->family, g->group);
		
		r = nl_socket_add_memberships(genlist->sock, g->gid, 0);
		if (r) {
			fprintf(stderr, "nl_socket_add_memberships failed: %d\n", r);
			nl_socket_free(genlist->sock);
			return 0;
		}
		g++;
	}
	
	return &genlist->parent;
}

void crtx_netlink_ge_init() {
}

void crtx_netlink_ge_finish() {
}





#ifdef CRTX_TEST

struct acpi_genl_event {
	char device_class[20];
	char bus_id[15];
	__u32 type;
	__u32 data;
};

void acpi_parse_cb(struct nlattr * attr, struct crtx_dict *dict) {
	struct acpi_genl_event *ev;
	struct crtx_dict_item *di;
	
	ev = (struct acpi_genl_event *) nla_data(attr);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 's', "device_class", ev->device_class, 0, CRTX_DIF_DONT_FREE_DATA);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 's', "bus_id", ev->bus_id, 0, CRTX_DIF_DONT_FREE_DATA);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "type", ev->type, sizeof(ev->type), 0);
	
	di = crtx_alloc_item(dict);
	crtx_fill_data_item(di, 'u', "data", ev->data, sizeof(ev->data), 0);
}

// call acpi_parse_cb for the second attribute
crtx_genl_parse_cb acpi_cbs[] = {
	0,
	&acpi_parse_cb,
};

// nl family and multicast group to bind to
struct crtx_genl_group genl_groups[] = {
	{ "acpi_event", "acpi_mc_group", 0, 0, 0, 0, acpi_cbs },
	{ 0 },
};

struct crtx_genl_callback genl_callbacks[] = {
	{ NL_CB_VALID, NL_CB_CUSTOM, crtx_genl_msg2dict, 0},
	{ 0 },
};


int netlink_main(int argc, char **argv) {
	struct crtx_genl_listener genl;
	struct crtx_listener_base *lbase;
	struct crtx_genl_group *g;
	int r;
	
	memset(&genl, 0, sizeof(struct crtx_genl_listener));
	
	genl_callbacks[0].arg = &genl;
	genl.callbacks = genl_callbacks;
	genl.groups = genl_groups;
	
	g = &genl_groups[0];
	// two attributes are defined but only the second is used
	g->n_attrs = 2;
	g->attrs = (struct nlattr**) calloc(1, sizeof(struct nlattr*)*g->n_attrs);
	g->attrs_template = crtx_create_dict("p", 
							"struct", 0, sizeof(struct acpi_genl_event), CRTX_DIF_DONT_FREE_DATA
						  );
	
	lbase = create_listener("genl", &genl);
	if (!lbase) {
		ERROR("create_listener(genl) failed\n");
		exit(1);
	}
	
	r = crtx_start_listener(lbase);
	if (!r) {
		ERROR("starting netlink_raw listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	return 0;
}

CRTX_TEST_MAIN(netlink_main);
#endif
