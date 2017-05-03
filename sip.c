
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
		
		add_event(slist->parent.graph, nevent);
		
// 		print_event_type(evt->type); print("\n");
		
// 		switch (evt->type) {
// 			case EXOSIP_REGISTRATION_SUCCESS:
// 				print("login success\n");
// 				break;
// 			case EXOSIP_REGISTRATION_FAILURE:
// 				print("login failed\n");
// 				break;
// 			case EXOSIP_CALL_INVITE:
// 			{
// 				char *tmp = NULL;
// 				osip_from_t *from;
// 				char *start, *at, *username, *display;
// 				int len;
// 				
// 				
// 				osip_from_to_str(evt->request->from, &tmp);
// 				
// 				// manually parse From line as the osip parser doesn't work here
// 				display = 0;
// 				start = strchr(tmp, '"');
// 				if (start) {
// 					start = start + 1;
// 					at = strchr(start, '"');
// 					if (at) {
// 						len = at - start + 1;
// 						display = (char*) malloc(len+1);
// 						strncpy(display, start, len);
// 						display[len-1] = 0;
// 					}
// 				}
// 				
// 				username = 0;
// 				start = strchr(tmp, '<')+1;
// 				if (start && !strncmp(start, "sip:", 4)) {
// 					at = strchr(start, '@');
// 					if (at) {
// 						len = at - start - 3;
// 						username = (char*) malloc(len+1);
// 						strncpy(username, start+4, len);
// 						username[len-1] = 0;
// 					}
// 				}
// 				
// 				print("new call %s %s %s \n", tmp, display, username);
// 				
// 				char notify[256], time_buffer[24];
// 				time_t timer;
// 				struct tm *tm_info;
// 				
// 				time(&timer);
// 				tm_info = localtime(&timer);
// 				
// 				strftime(time_buffer, 24, "%H:%M:%S", tm_info);
// 				snprintf(notify, 255, format, display, username, time_buffer);
// 				system(notify);
// 				
// 				osip_free(tmp);
// 				
// 				if (display)
// 					free(display);
// 				if (username)
// 					free(username);
// 			}
// 			break;
// 			default:
// 				print("unhandled event %d\n", evt->type);
// 		}
		
// 		eXosip_event_free(evt);
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

#include "sip_eXosip_event2dict.c"

static char sip_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;
	struct eXosip_event *evt;
	
	dict = 0;
	evt = (struct eXosip_event *) event->data.pointer;
	
	if (evt->type == EXOSIP_CALL_INVITE) {
		char *displayname, *username;
		char r;
		
		crtx_eXosip_event2dict(evt, &dict);
		
// 		crtx_print_dict(dict);
		
		r = crtx_dict_locate_value(dict, "request/from/displayname", 's', &displayname, sizeof(displayname));
		r |= crtx_dict_locate_value(dict, "request/from/url/username", 's', &username, sizeof(username));
		if (r == 0)
			printf("%s %s\n", displayname, username);
		
		crtx_dict_unref(dict);
	}
	
	return 0;
}

int sip_main(int argc, char **argv) {
	struct crtx_sip_listener slist;
	struct crtx_listener_base *lbase;
	char ret;
	
	TRACE_INITIALIZE (6, NULL);
	
	memset(&slist, 0, sizeof(struct crtx_sip_listener));
	
	slist.transport = IPPROTO_UDP;
	slist.src_addr = 0;
	slist.src_port = 5061;
	slist.family = AF_INET;
	slist.secure = 0;
	
	slist.username = "620";
	slist.userid = "620";
	slist.password = "mariomario";
	slist.dst_addr = "fritz.box";
	
	lbase = create_listener("sip", &slist);
	if (!lbase) {
		ERROR("create_listener(sip) failed\n");
		exit(1);
	}
	
	crtx_create_task(lbase->graph, 0, "sip_test", sip_test_handler, 0);
	
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
