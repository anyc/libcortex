
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <netdb.h>


#include "core.h"
#include "nf_queue.h"
#include "plugins.h"
#include "sd_bus.h"

struct event_task *task, *rp_cache;
struct event_graph *rl_graph;

static void handle(struct event *event, void *userdata) {
	struct ev_nf_queue_packet_msg *ev;
	struct iphdr *iph;
	struct event *rl_event;
	char *s;
// 	size_t len;
// 	char *proto, *msg;
	char buf[64];
	
	if (event->response)
		return;
	
	if (!nfq_packet_msg_okay(event))
		return;
	
	ev = (struct ev_nf_queue_packet_msg*) event->data;
	
	iph = (struct iphdr*)ev->payload;
	
// 	ss = inet_ntoa(*(struct in_addr *)&iph->saddr);
// 	sd = inet_ntoa(*(struct in_addr *)&iph->daddr);
// 	
// 	proto = nfq_proto2str(iph->protocol);
// 	msg = "(yes/no)";
// 	len = strlen(proto) + 1 + strlen(ss) + 1 + strlen(sd) + 1 + strlen(msg) + 1;
// 	s = (char*) malloc(len);
// 	sprintf(s, "%s %s %s %s", proto, ss, sd, msg);
	
	int res;
// 	struct sockaddr_in sa;
	char host[64];
	
// 	memset(&sa, 0, sizeof(struct sockaddr));
	
// 	sa.sin_addr.s_addr = iph->saddr;
// 	struct sockaddr_in sa = { .sin_family = AF_INET, .sin_addr = &iph->saddr };
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr = *(struct in_addr *)&iph->saddr;
	res = getnameinfo((struct sockaddr*) &sa, sizeof(struct sockaddr), host, 64, 0, 0, 0);
	if (res) {
		printf("getnameinfo failed %d\n", res);
		host[0] = 0;
	}
	
	s = buf;
	s += sprintf(s, "%s %s (%s) ", nfq_proto2str(iph->protocol),
				host,
				inet_ntoa(*(struct in_addr *)&iph->saddr));
	
	sa.sin_addr = *(struct in_addr *)&iph->daddr;
	res = getnameinfo((struct sockaddr*) &sa, sizeof(struct sockaddr), host, 64, 0, 0, 0);
	if (res) {
		printf("getnameinfo failed %d\n", res);
		host[0] = 0;
	}
	
	s += sprintf(s, "%s (%s) (yes/no)",
				host,
				inet_ntoa(*(struct in_addr *)&iph->daddr));
	
	
	while (!event->response) {
		rl_event = new_event();
		rl_event->type = "readline/request";
		rl_event->data = buf;
		rl_event->data_size = s-buf;
		
		if (!rl_graph)
			rl_graph = get_graph_for_event_type("readline/request");
		
		add_event(rl_graph, rl_event);
		
		wait_on_event(rl_event);
		
		if (rl_event->response && !strncmp(rl_event->response, "yes", 3)) {
			event->response = (void*) 1;
		}
		if (rl_event->response && !strncmp(rl_event->response, "no", 2)) {
			event->response = (void*) 2;
		}
	}
}

u_int32_t send_notification(char *icon, char *title, char *text, char **actions) {
	int r;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *reply = NULL;
	char **ait;
	sd_bus *bus;

	r = sd_bus_open_user(&bus);
	if (r < 0) {
		printf("Failed to connect to system bus: %s\n", strerror(-r));
		return 0;
	}
	
	// 	r = sd_bus_message_new_method_call(bus, &m, dest, path, interface, member);
	r = sd_bus_message_new_method_call(bus,
								&m,
								"org.freedesktop.Notifications",           /* service to contact */
								"/org/freedesktop/Notifications",          /* object path */
								"org.freedesktop.Notifications",   /* interface name */
								"Notify"                         /* method name */
							);
	if (r < 0) {
		fprintf(stderr, "Failed to issue method call: %s %s\n", error.name, error.message);
		return 0;
	}

	#define SDCHECK(r) { if (r < 0) {printf("err %d %s (%d %d) %d\n", r, strerror(r), EINVAL, EPERM,  __LINE__);} }
	r = sd_bus_message_append_basic(m, 's', "cortexd"); SDCHECK(r)
	int32_t id = 0;
	r = sd_bus_message_append_basic(m, 'u', &id); SDCHECK(r)
	r = sd_bus_message_append_basic(m, 's', icon); SDCHECK(r)
	r = sd_bus_message_append_basic(m, 's', title); SDCHECK(r)
	r = sd_bus_message_append_basic(m, 's', text); SDCHECK(r)

	r = sd_bus_message_open_container(m, 'a', "s"); SDCHECK(r)
	if (actions) {
		ait = actions;
		while (*ait) {
			r = sd_bus_message_append_basic(m, 's', *ait); SDCHECK(r)
			ait++;
		}
	} else {
		r = sd_bus_message_append_basic(m, 's', ""); SDCHECK(r)
	}
	r = sd_bus_message_close_container(m); SDCHECK(r)

	r = sd_bus_message_open_container(m, 'a', "{sv}"); SDCHECK(r)
	r = sd_bus_message_open_container(m, 'e', "sv"); SDCHECK(r)
	r = sd_bus_message_append_basic(m, 's', ""); SDCHECK(r)

	r = sd_bus_message_open_container(m, 'v', "s"); SDCHECK(r)
	r = sd_bus_message_append_basic(m, 's', ""); SDCHECK(r)
	r = sd_bus_message_close_container(m); SDCHECK(r)
	r = sd_bus_message_close_container(m); SDCHECK(r)
	r = sd_bus_message_close_container(m); SDCHECK(r)

	int32_t time = 0;
	r = sd_bus_message_append_basic(m, 'i', &time); SDCHECK(r)

	r = sd_bus_call(bus, m, -1, &error, &reply); SDCHECK(r)

	u_int32_t res;
	r = sd_bus_message_read(reply, "u", &res);
	if (r < 0) {
		fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
	}
	
	return res;
}

void handle_notification_response(struct event *event, void *userdata, void **sessiondata) {
	sd_bus_message *m = (sd_bus_message *) event->data;
	
	sd_bus_print_msg(m);
}


void handle_cortexd_module_initialized(struct event *event, void *userdata, void *sessiondata) {
	printf("handle_cortexd_module_initialized\n");
}


#define EV_NOTIF_AC_INVOKED "sd-bus.org.freedesktop.Notifications.ActionInvoked"
char *notif_event_types[] = { EV_NOTIF_AC_INVOKED, 0 };

void send_query(char *icon, char *title, char *text, char **actions, char**chosen_action) {
	u_int32_t id;
	
	id = send_notification(icon, title, text, actions);
	
	struct event_graph *graph;
	
	graph = get_graph_for_event_type(EV_NOTIF_AC_INVOKED);
	if (!graph) {
		new_eventgraph(&graph, notif_event_types);
	}
	
	sd_bus_add_listener(sd_bus_main_bus, "/org/freedesktop/Notifications", EV_NOTIF_AC_INVOKED);
	
	struct event_task *task2;
	task2 = new_task();
	task2->id = "notif_handler";
	task2->handle = &handle_notification_response;
	task2->userdata = 0;
	add_task(graph, task2);
	
	
	
// 	struct event_task *etask = new_task();
// 	etask->handle = &handle_cortexd_enable;
	
}

void init() {
	char *actions[] = { "test1", "test2", "test3", "test4", 0 };
	printf("send\n");
	send_notification("asd", "title", "text", actions);
	
	return;
	
	struct nfq_thread_data *td;
	
	td = (struct nfq_thread_data*) create_listener("nf_queue", 0);
	
	rp_cache = create_response_cache_task(&nfq_decision_cache_create_key);
	add_task(td->graph, rp_cache);
	
	task = new_task();
	task->id = "pfw_handler";
	task->handle = &handle;
	task->userdata = td;
	add_task(td->graph, task);
	
// 	print_tasks(td->graph);
	
// 	sleep(1);
// 	
// 	struct event_graph *graph;
// 	char *event_types[] = { "sd-bus.org.freedesktop.Notifications.ActionInvoked", 0 };
// 	new_eventgraph(&graph, event_types);
// 	
// 	// 	 "org.freedesktop.Notifications.ActionInvoked"
// 	sd_bus_add_listener(sd_bus_main_bus, "/org/freedesktop/Notifications", "sd-bus.org.freedesktop.Notifications.ActionInvoked");
// 	
// 	struct event_task *task2;
// 	task2 = new_task();
// 	task2->id = "pfw_handler";
// 	task2->handle = &sd_bus_print_event_task;
// 	task2->userdata = td;
// 	add_task(graph, task2);
	
	
}

void finish() {
	
}