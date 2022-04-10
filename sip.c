/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eXosip2/eXosip.h>

#include "intern.h"
#include "core.h"
#include "sip.h"

#include "sip_eXosip_event2dict.c"

static void sip_event_before_release_cb(struct crtx_event *event, void *userdata) {
	eXosip_event_t *evt;
	
	evt = (eXosip_event_t *) crtx_event_get_ptr(event);
	eXosip_event_free(evt);
}

static char sip_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_sip_listener *slist;
	eXosip_event_t *evt;
	struct crtx_event *nevent;
	int r;
	
	
	slist = (struct crtx_sip_listener *) userdata;
	
	while(1) {
		evt = eXosip_event_wait(slist->ctx, 0, 0);
		
		eXosip_lock(slist->ctx);
		eXosip_automatic_action(slist->ctx);
		eXosip_unlock(slist->ctx);
		
		if (!evt)
			break;
		
		r = crtx_create_event(&nevent); // evt, sizeof(eXosip_event_t*));
		if (r) {
			CRTX_ERROR("crtx_create_event() failed: %s\n", strerror(-r));
			return r;
		}
		
		crtx_event_set_raw_data(nevent, 'p', evt, sizeof(eXosip_event_t*), CRTX_DIF_DONT_FREE_DATA);
		
		nevent->cb_before_release = &sip_event_before_release_cb;
		
		crtx_add_event(slist->base.graph, nevent);
	}
	
	return 0;
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_sip_listener *slist;
	int r;
	osip_message_t *reg;
	char from[256], proxy[256];
	
	
	slist = (struct crtx_sip_listener *) listener;
	
	snprintf(from, 255, "sip:%s@%s", slist->username, slist->dst_addr);
	snprintf(proxy, 255, "sip:%s", slist->dst_addr);
	
	
	eXosip_lock(slist->ctx);
	
	reg = 0;
	slist->rid = eXosip_register_build_initial_register(slist->ctx, from, proxy, NULL, 1800, &reg);
	if (slist->rid < 0) {
		CRTX_ERROR("eXosip_register_build_initial_register failed: %d %s\n", slist->rid, osip_strerror(slist->rid));
		eXosip_unlock(slist->ctx);
		return slist->rid;
	}
	
	osip_message_set_supported(reg, "100rel");
	osip_message_set_supported(reg, "path");
	r = eXosip_register_send_register(slist->ctx, slist->rid, reg);
	if (r) {
		CRTX_ERROR("eXosip_register_send_register failed: %d\n", r);
		eXosip_unlock(slist->ctx);
		return r;
	}
	eXosip_unlock (slist->ctx);
	
	crtx_evloop_init_listener(&slist->base,
						eXosip_event_geteventsocket(slist->ctx),
						CRTX_EVLOOP_READ,
						0,
						&sip_fd_event_handler,
						slist,
						0,
						0
					);
	
	return 0;
}

static char stop_listener(struct crtx_listener_base *listener) {
	osip_message_t *reg = NULL;
	int r;
	struct crtx_sip_listener *slist;
	
	
	slist = (struct crtx_sip_listener *) listener;
	
	eXosip_lock(slist->ctx);
	r = eXosip_register_build_register(slist->ctx, slist->rid, 0, &reg);
	if (r < 0) {
		CRTX_ERROR("eXosip_register_build_register failed: %d\n", r);
		eXosip_unlock(slist->ctx);
		return r;
	}
	
	eXosip_register_send_register(slist->ctx, slist->rid, reg);
	eXosip_unlock(slist->ctx);
	
	return 0;
}

static void free_listener(struct crtx_listener_base *listener, void *userdata) {
	struct crtx_sip_listener *slist;
	
	slist = (struct crtx_sip_listener *) listener;
	
	eXosip_quit(slist->ctx);
}

struct crtx_listener_base *crtx_setup_sip_listener(void *options) {
	struct crtx_sip_listener *slist;
	int r;
	
	
	slist = (struct crtx_sip_listener *) options;
	
// 	TRACE_INITIALIZE(6, NULL);
	
	slist->ctx = eXosip_malloc();
	if (!slist->ctx) {
		CRTX_ERROR("eXosip_malloc failed\n");
		return 0;
	}
	
	r = eXosip_init(slist->ctx);
	if (r) {
		CRTX_ERROR("eXosip_init failed: %d\n", r);
		return 0;
	}
	
	r = eXosip_listen_addr(slist->ctx, slist->transport, slist->src_addr, slist->src_port, slist->family, slist->secure);
	if (r) {
		eXosip_quit(slist->ctx);
		CRTX_ERROR("eXosip_listen_addr failed: %d\n", r);
		return 0;
	}
	
	eXosip_lock(slist->ctx);
	eXosip_add_authentication_info(slist->ctx, slist->username, slist->userid, slist->password, NULL, NULL);
	eXosip_unlock(slist->ctx);
	
	slist->base.start_listener = &start_listener;
	slist->base.stop_listener = &stop_listener;
	slist->base.free_cb = &free_listener;
	
	return &slist->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(sip)

void crtx_sip_init() {
}

void crtx_sip_finish() {
}



#ifdef CRTX_TEST

#include <netinet/in.h>
#include <unistd.h>


void help(char **argv) {
	fprintf(stderr, "Usage: %s [options]\n", argv[0]);
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "  -s <server>   \n");
	fprintf(stderr, "  -u <user>     \n");
	fprintf(stderr, "  -p <password> \n");
}

struct crtx_dict_transformation dict_transformation[] = {
	{ "title", 's', 0, "New phone call from %[request/from/displayname]s" },
	{ "message", 's', 0, "%[request/from/displayname]s (%[request/from/url/username]s) called" },
	{ "icon", 's', 0, "phone" },
};

static char sip2notify_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;
	struct eXosip_event *evt;
	struct crtx_graph *notify_graph;
	int r;
	
	
	dict = 0;
	evt = (struct eXosip_event *) event->data.pointer;
	
	notify_graph = (struct crtx_graph *) userdata;
	
	if (evt->type == EXOSIP_CALL_INVITE) {
		struct crtx_event *notify_event;
		struct crtx_dict *notify_dict;
		
		crtx_eXosip_event2dict(evt, &dict);
		
// 		char *displayname, *username;
// 		char r;
// 		r = crtx_dict_locate_value(dict, "request/from/displayname", 's', &displayname, sizeof(displayname));
// 		r |= crtx_dict_locate_value(dict, "request/from/url/username", 's', &username, sizeof(username));
// 		if (r == 0 && displayname && username)
// 			crtx_print_dict(notify_dict);
			
		r = crtx_create_event(&notify_event);
		if (r) {
			CRTX_ERROR("crtx_create_event() failed: %s\n", strerror(-r));
			return r;
		}
		notify_dict = crtx_dict_transform(dict, "sss", dict_transformation);
		
		crtx_event_set_dict_data(notify_event, notify_dict, 0);
		crtx_add_event(notify_graph, notify_event);
		
		crtx_dict_unref(dict);
	}
	
	return 0;
}

int sip_main(int argc, char **argv) {
	struct crtx_sip_listener slist;
	struct crtx_graph *notify_graph;
	char ret, opt;
	char *username, *password, *server;
	int r;
	
	
	username = password = server = 0;
	while ((opt = getopt (argc, argv, "hu:p:s:")) != -1) {
		switch (opt) {
			case 'u':
				username = optarg;
				break;
			case 'p':
				password = optarg;
				break;
			case 's':
				server = optarg;
				break;
			default:
				help(argv);
				return 1;
		}
	}
	
	if (!username || !password || !server) {
		help(argv);
		return 1;
	}
	
	
	memset(&slist, 0, sizeof(struct crtx_sip_listener));
	
	slist.transport = IPPROTO_UDP;
	slist.src_addr = 0;
	slist.src_port = 5061;
	slist.family = AF_INET;
	slist.secure = 0;
	
	slist.username = username;
	slist.userid = username;
	slist.password = password;
	slist.dst_addr = server;
	
	r = crtx_setup_listener("sip", &slist);
	if (r) {
		CRTX_ERROR("create_listener(sip) failed\n");
		exit(1);
	}
	
	// create a new graph and add all known task handlers for the event type "user_notification"
	crtx_create_graph(&notify_graph, "notify_graph");
	crtx_autofill_graph_with_tasks(notify_graph, "cortex.user_notification");
	
	// add the handler that will transform sip events into user notification events
	crtx_create_task(slist.base.graph, 0, "sip2notify", sip2notify_handler, notify_graph);
	
	ret = crtx_start_listener(&slist.base);
	if (ret) {
		CRTX_ERROR("starting sip listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	crtx_shutdown_listener(&slist.base);
	
	return 0;  
}

CRTX_TEST_MAIN(sip_main);

#endif
