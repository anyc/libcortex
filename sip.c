
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <eXosip2/eXosip.h>

#include "core.h"
#include "sip.h"

void print(char*s) { printf("%s\n", s); }
void print_event_type(int type) {
	switch (type) {
		case EXOSIP_REGISTRATION_SUCCESS: print("EXOSIP_REGISTRATION_SUCCESS"); break;
		case EXOSIP_REGISTRATION_FAILURE: print("EXOSIP_REGISTRATION_FAILURE"); break;
		
		case EXOSIP_CALL_INVITE: print("EXOSIP_CALL_INVITE"); break;
		case EXOSIP_CALL_REINVITE: print("EXOSIP_CALL_REINVITE"); break;
		
		case EXOSIP_CALL_NOANSWER: print("EXOSIP_CALL_NOANSWER"); break;
		case EXOSIP_CALL_PROCEEDING: print("EXOSIP_CALL_PROCEEDING"); break;
		case EXOSIP_CALL_RINGING: print("EXOSIP_CALL_RINGING"); break;
		case EXOSIP_CALL_ANSWERED: print("EXOSIP_CALL_ANSWERED"); break;
		case EXOSIP_CALL_REDIRECTED: print("EXOSIP_CALL_REDIRECTED"); break;
		case EXOSIP_CALL_REQUESTFAILURE: print("EXOSIP_CALL_REQUESTFAILURE"); break;
		case EXOSIP_CALL_SERVERFAILURE: print("EXOSIP_CALL_SERVERFAILURE"); break;
		case EXOSIP_CALL_GLOBALFAILURE: print("EXOSIP_CALL_GLOBALFAILURE"); break;
		case EXOSIP_CALL_ACK: print("EXOSIP_CALL_ACK"); break;
		case EXOSIP_CALL_CANCELLED: print("EXOSIP_CALL_CANCELLED"); break;
		
		case EXOSIP_CALL_MESSAGE_NEW: print("EXOSIP_CALL_MESSAGE_NEW"); break;
		case EXOSIP_CALL_MESSAGE_PROCEEDING: print("EXOSIP_CALL_MESSAGE_PROCEEDING"); break;
		case EXOSIP_CALL_MESSAGE_ANSWERED: print("EXOSIP_CALL_MESSAGE_ANSWERED"); break;
		case EXOSIP_CALL_MESSAGE_REDIRECTED: print("EXOSIP_CALL_MESSAGE_REDIRECTED"); break;
		case EXOSIP_CALL_MESSAGE_REQUESTFAILURE: print("EXOSIP_CALL_MESSAGE_REQUESTFAILURE"); break;
		case EXOSIP_CALL_MESSAGE_SERVERFAILURE: print("EXOSIP_CALL_MESSAGE_SERVERFAILURE"); break;
		case EXOSIP_CALL_MESSAGE_GLOBALFAILURE: print("EXOSIP_CALL_MESSAGE_GLOBALFAILURE"); break;
		case EXOSIP_CALL_CLOSED: print("EXOSIP_CALL_CLOSED"); break;
		case EXOSIP_CALL_RELEASED: print("EXOSIP_CALL_RELEASED"); break;
		
		case EXOSIP_MESSAGE_NEW: print("EXOSIP_MESSAGE_NEW"); break;
		case EXOSIP_MESSAGE_PROCEEDING: print("EXOSIP_MESSAGE_PROCEEDING"); break;
		case EXOSIP_MESSAGE_ANSWERED: print("EXOSIP_MESSAGE_ANSWERED"); break;
		case EXOSIP_MESSAGE_REDIRECTED: print("EXOSIP_MESSAGE_REDIRECTED"); break;
		case EXOSIP_MESSAGE_REQUESTFAILURE: print("EXOSIP_MESSAGE_REQUESTFAILURE"); break;
		case EXOSIP_MESSAGE_SERVERFAILURE: print("EXOSIP_MESSAGE_SERVERFAILURE"); break;
		case EXOSIP_MESSAGE_GLOBALFAILURE: print("EXOSIP_MESSAGE_GLOBALFAILURE"); break;
		
		case EXOSIP_SUBSCRIPTION_NOANSWER: print("EXOSIP_SUBSCRIPTION_NOANSWER"); break;
		case EXOSIP_SUBSCRIPTION_PROCEEDING: print("EXOSIP_SUBSCRIPTION_PROCEEDING"); break;
		case EXOSIP_SUBSCRIPTION_ANSWERED: print("EXOSIP_SUBSCRIPTION_ANSWERED"); break;
		case EXOSIP_SUBSCRIPTION_REDIRECTED: print("EXOSIP_SUBSCRIPTION_REDIRECTED"); break;
		case EXOSIP_SUBSCRIPTION_REQUESTFAILURE: print("EXOSIP_SUBSCRIPTION_REQUESTFAILURE"); break;
		case EXOSIP_SUBSCRIPTION_SERVERFAILURE: print("EXOSIP_SUBSCRIPTION_SERVERFAILURE"); break;
		case EXOSIP_SUBSCRIPTION_GLOBALFAILURE: print("EXOSIP_SUBSCRIPTION_GLOBALFAILURE"); break;
		case EXOSIP_SUBSCRIPTION_NOTIFY: print("EXOSIP_SUBSCRIPTION_NOTIFY"); break;
		
		case EXOSIP_IN_SUBSCRIPTION_NEW: print("EXOSIP_IN_SUBSCRIPTION_NEW"); break;
		
		case EXOSIP_NOTIFICATION_NOANSWER: print("EXOSIP_NOTIFICATION_NOANSWER"); break;
		case EXOSIP_NOTIFICATION_PROCEEDING: print("EXOSIP_NOTIFICATION_PROCEEDING"); break;
		case EXOSIP_NOTIFICATION_ANSWERED: print("EXOSIP_NOTIFICATION_ANSWERED"); break;
		case EXOSIP_NOTIFICATION_REDIRECTED: print("EXOSIP_NOTIFICATION_REDIRECTED"); break;
		case EXOSIP_NOTIFICATION_REQUESTFAILURE: print("EXOSIP_NOTIFICATION_REQUESTFAILURE"); break;
		case EXOSIP_NOTIFICATION_SERVERFAILURE: print("EXOSIP_NOTIFICATION_SERVERFAILURE"); break;
		case EXOSIP_NOTIFICATION_GLOBALFAILURE: print("EXOSIP_NOTIFICATION_GLOBALFAILURE"); break;
		default:
			print("TODO\n");
	}
}

void sip_event_before_release_cb(struct crtx_event *event) {
	eXosip_event_t *evt;
	
	evt = (eXosip_event_t *) crtx_event_get_ptr(event);
	eXosip_event_free(evt);
}


static char sip_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *payload;
	struct crtx_sip_listener *slist;
	eXosip_event_t *evt;
	struct crtx_event *nevent;
	
	
	payload = (struct crtx_event_loop_payload*) event->data.pointer;
	
	slist = (struct crtx_sip_listener *) payload->data;
	
	while(1) {
		evt = eXosip_event_wait(slist->ctx, 0, 0);
		
		eXosip_lock(slist->ctx);
		eXosip_automatic_action(slist->ctx);
		eXosip_unlock(slist->ctx);
		
		if (!evt)
			break;
		
		nevent = crtx_create_event(0, evt, sizeof(eXosip_event_t*));
		nevent->data.flags |= CRTX_DIF_DONT_FREE_DATA;
		
		nevent->cb_before_release = &sip_event_before_release_cb;
		
		crtx_add_event(slist->parent.graph, nevent);
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
	// 	r = initial_register(ctx, from, proxy, &rid);
	
	
	eXosip_lock(slist->ctx);
	
	reg = 0;
	slist->rid = eXosip_register_build_initial_register(slist->ctx, from, proxy, NULL, 1800, &reg);
	if (slist->rid < 0) {
		ERROR("eXosip_register_build_initial_register failed: %d %s\n", slist->rid, osip_strerror(slist->rid));
		eXosip_unlock(slist->ctx);
		return 1;
	}
	
	osip_message_set_supported(reg, "100rel");
	osip_message_set_supported(reg, "path");
	r = eXosip_register_send_register(slist->ctx, slist->rid, reg);
	if (r) {
		ERROR("eXosip_register_send_register failed: %d\n", r);
		eXosip_unlock(slist->ctx);
		return 1;
	}
	eXosip_unlock (slist->ctx);
	
	slist->parent.el_payload.fd = eXosip_event_geteventsocket(slist->ctx);
	slist->parent.el_payload.event_flags = EPOLLIN;
	slist->parent.el_payload.data = slist;
	slist->parent.el_payload.event_handler = &sip_fd_event_handler;
	slist->parent.el_payload.event_handler_name = "sip fd handler";
	
	return 0;
}

int stop_listener(struct crtx_listener_base *listener) {
	osip_message_t *reg = NULL;
	int r;
	struct crtx_sip_listener *slist;
	
	slist = (struct crtx_sip_listener *) listener;
	
	
	eXosip_lock(slist->ctx);
	r = eXosip_register_build_register(slist->ctx, slist->rid, 0, &reg);
	if (r < 0) {
		ERROR("eXosip_register_build_register failed: %d\n", r);
		eXosip_unlock(slist->ctx);
		return 1;
	}
	
	eXosip_register_send_register(slist->ctx, slist->rid, reg);
	eXosip_unlock(slist->ctx);
	
	return 0;
}

static void free_listener(struct crtx_listener_base *listener, void *userdata) {
	struct crtx_sip_listener *slist;
// 	int r;
	
	slist = (struct crtx_sip_listener *) listener;
	
	eXosip_quit(slist->ctx);
}

struct crtx_listener_base *crtx_new_sip_listener(void *options) {
	struct crtx_sip_listener *slist;
	int r;
	
	slist = (struct crtx_sip_listener *) options;
	
	slist->ctx = eXosip_malloc();
	if (!slist->ctx) {
		ERROR("eXosip_malloc failed\n");
		return 0;
	}
	
	r = eXosip_init(slist->ctx);
	if (r) {
		ERROR("eXosip_init failed: %d\n", r);
		return 0;
	}
	
	r = eXosip_listen_addr(slist->ctx, slist->transport, slist->src_addr, slist->src_port, slist->family, slist->secure);
	if (r) {
		eXosip_quit(slist->ctx);
		ERROR("eXosip_listen_addr failed: %d\n", r);
		return 0;
	}
	
	eXosip_lock(slist->ctx);
	eXosip_add_authentication_info(slist->ctx, slist->username, slist->userid, slist->password, NULL, NULL);
	eXosip_unlock(slist->ctx);
	
	slist->parent.start_listener = &start_listener;
	slist->parent.free = &free_listener;
	
	return &slist->parent;
}

void crtx_sip_init() {
}

void crtx_sip_finish() {
}



#ifdef CRTX_TEST

#include <netinet/in.h>
#include <unistd.h>

#include "sip_eXosip_event2dict.c"

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
	// key, type, flag, format
	{ "title", 's', 0, "New phone call" },
	{ "message", 's', 0, "New call from %[request/from/displayname]s %[request/from/url/username]s" },
};

static char sip_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;
	struct eXosip_event *evt;
	struct crtx_graph *notify_graph;
	
	dict = 0;
	evt = (struct eXosip_event *) event->data.pointer;
	
	notify_graph = (struct crtx_graph *) userdata;
	
	if (evt->type == EXOSIP_CALL_INVITE) {
		char *displayname, *username, *msg;
// 		size_t msg_len;
		char r;
		struct crtx_event *notify_event;
		struct crtx_dict *notify_dict;
		
		crtx_eXosip_event2dict(evt, &dict);
		
// 		crtx_print_dict(dict);
		
		r = crtx_dict_locate_value(dict, "request/from/displayname", 's', &displayname, sizeof(displayname));
		r |= crtx_dict_locate_value(dict, "request/from/url/username", 's', &username, sizeof(username));
		if (r == 0 && displayname && username) {
// 			printf("%s %s\n", displayname, username);
			
// 			msg = (char *) malloc(strlen(displayname) + strlen(username)
			
			notify_event = crtx_create_event(0, 0, 0);
// 			nevent->data.flags |= CRTX_DIF_DONT_FREE_DATA;
// 			nevent->cb_before_release = &sip_event_before_release_cb;
			
// 			dict = crtx_create_dict("ss", 
// 				's', "title", "new call", sizeof("new call"), CRTX_DIF_DONT_FREE_DATA,
// 				's', "message", msg, msg_len, 0,
// 				);
			
			notify_dict = crtx_dict_transform(dict, "ss", dict_transformation);
			crtx_print_dict(notify_dict);
			crtx_event_set_data(notify_event, 0, notify_dict, 0);
			
			crtx_add_event(notify_graph, notify_event);
		}
		
		crtx_dict_unref(dict);
	}
	
	return 0;
}

int sip_main(int argc, char **argv) {
	struct crtx_sip_listener slist;
	struct crtx_listener_base *lbase;
	struct crtx_graph *notify_graph;
	char ret, opt;
	char *username, *password, *server;
	
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
	
	
	TRACE_INITIALIZE (6, NULL);
	
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
	
	lbase = create_listener("sip", &slist);
	if (!lbase) {
		ERROR("create_listener(sip) failed\n");
		exit(1);
	}
	
	notify_graph = 0;
	crtx_create_graph(&notify_graph, "notify_graph", 0);
	crtx_autofill_graph_with_tasks("user_notification", notify_graph);
	
	crtx_create_task(lbase->graph, 0, "sip_test", sip_test_handler, notify_graph);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting sip listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	return 0;  
}

CRTX_TEST_MAIN(sip_main);

#endif
