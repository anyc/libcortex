
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include <avahi-client/client.h>
// #include <avahi-client/publish.h>

// #include <avahi-common/simple-watch.h>
// #include <avahi-common/error.h>
// #include <avahi-common/malloc.h>

#include "avahi.h"
#include "sd_bus.h"


int crtx_avahi_resolve_service(struct crtx_avahi_service *service) {
	int r;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *reply = NULL;
	struct crtx_sdbus_listener *slist;
	size_t size;
	
	slist = crtx_sdbus_get_default_listener(CRTX_SDBUS_TYPE_SYSTEM);
	
	r = sd_bus_message_new_method_call(slist->bus,
									&m,
									"org.freedesktop.Avahi",
									"/",
									"org.freedesktop.Avahi.Server",
									"ResolveService"
	);
	if (r < 0) {
		ERROR("Failed to create method call: %s %s\n", error.name, error.message);
		return r;
	}
	
	r = sd_bus_message_append_basic(m, 'i', &service->interface); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 'i', &service->protocol); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', service->name); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', service->type); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', service->domain); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 'i', &service->aprotocol); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 'u', &service->flags); CRTX_RET_GEZ(r)
	
	r = sd_bus_call(slist->bus, m, -1, &error, &reply); 
	if (r < 0) {
		ERROR("Failed to issue method call: %s %s\n", error.name, error.message);
		return r;
	}
	
	sd_bus_message_unref(m);
	
	r = sd_bus_message_read_basic(reply, 'i', &service->interface); CRTX_RET_GEZ(r)
	r = sd_bus_message_read_basic(reply, 'i', &service->protocol); CRTX_RET_GEZ(r)
	r = crtx_sd_bus_message_read_string(reply, &service->name); CRTX_RET_GEZ(r)
	r = crtx_sd_bus_message_read_string(reply, &service->type); CRTX_RET_GEZ(r)
	r = crtx_sd_bus_message_read_string(reply, &service->domain); CRTX_RET_GEZ(r)
	r = crtx_sd_bus_message_read_string(reply, &service->host); CRTX_RET_GEZ(r)
	r = sd_bus_message_read_basic(reply, 'i', &service->aprotocol); CRTX_RET_GEZ(r)
	r = crtx_sd_bus_message_read_string(reply, &service->address); CRTX_RET_GEZ(r)
	r = sd_bus_message_read_basic(reply, 'q', &service->port); CRTX_RET_GEZ(r)
	
// 	r = sd_bus_message_read_array(reply, 'y', &service->txt, &size); CRTX_RET_GEZ(r)
// 	r = sd_bus_message_enter_container(reply, 'a', "ay");CRTX_RET_GEZ(r)
	while ((r = sd_bus_message_enter_container(reply, 'a', "ay")) > 0) {
		char *s;
		s = 0; size = 0;
// 		r = sd_bus_message_enter_container(reply, 'a', "y");CRTX_RET_GEZ(r)
		r = sd_bus_message_read_array(reply, 'y', (const void**) &s, &size); CRTX_RET_GEZ(r)
		if (size > 0) {
			printf("string %p %zu\n", s, size);
			printf("string %s %zu\n", s, size);
		}
		
// 		r = sd_bus_message_exit_container(reply); CRTX_RET_GEZ(r)
		r = sd_bus_message_exit_container(reply); CRTX_RET_GEZ(r)
	}
	
	r = sd_bus_message_read_basic(reply, 'u', &service->flags); CRTX_RET_GEZ(r)
	
	sd_bus_message_unref(reply);
	
	return 0;
}

char crtx_avahi_publish_service(struct crtx_avahi_service *service) {
	int r;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *reply = NULL;
	struct crtx_sdbus_listener *slist;
	
	
	slist = crtx_sdbus_get_default_listener(CRTX_SDBUS_TYPE_SYSTEM);
	
	r = sd_bus_message_new_method_call(slist->bus,
									&m,
									"org.freedesktop.Avahi",        /* service to contact */
									"/",                            /* object path */
									"org.freedesktop.Avahi.Server", /* interface name */
									"EntryGroupNew"                 /* method name */
								);
	CRTX_RET_GEZ(r);
	
	r = sd_bus_call(slist->bus, m, -1, &error, &reply);
	if (r < 0) {
		ERROR("Failed to issue method call: %s %s\n", error.name, error.message);
		return r;
	}
	
	r = sd_bus_message_read(reply, "o", &service->payload); CRTX_RET_GEZ(r);
	service->payload = crtx_stracpy(service->payload, 0);
	
	sd_bus_message_unref(reply);
	sd_bus_message_unref(m);
	
	
	
	r = sd_bus_message_new_method_call(slist->bus,
									&m,
									"org.freedesktop.Avahi",
									service->payload,
									"org.freedesktop.Avahi.EntryGroup",
									"AddService"
								);
	if (r < 0) {
		ERROR("Failed to issue method call: %s %s\n", error.name, error.message);
		return r;
	}
	
	r = sd_bus_message_append_basic(m, 'i', &service->interface); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 'i', &service->protocol); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 'u', &service->flags); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', service->name); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', service->type); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', service->domain); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', service->host); CRTX_RET_GEZ(r)
	uint32_t port = service->port;
	r = sd_bus_message_append_basic(m, 'q', &port); CRTX_RET_GEZ(r)
	r = sd_bus_message_append(m, "aay", 0); CRTX_RET_GEZ(r)
	
	r = sd_bus_call(slist->bus, m, -1, &error, 0); 
	if (r < 0) {
		ERROR("Failed to issue method call: %s %s\n", error.name, error.message);
		return r;
	}
	
	sd_bus_message_unref(m);
	
	
	
	r = sd_bus_message_new_method_call(slist->bus,
									&m,
									"org.freedesktop.Avahi",
									service->payload,
									"org.freedesktop.Avahi.EntryGroup",
									"Commit"
								);
	CRTX_RET_GEZ(r);
	
	r = sd_bus_call(slist->bus, m, -1, &error, 0);
	if (r < 0) {
		ERROR("Failed to issue method call: %s %s\n", error.name, error.message);
		return r;
	}
	
	sd_bus_message_unref(m);
	
	return 0;
}

char crtx_avahi_remove_service(struct crtx_avahi_service *service) {
	int r;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *reply = NULL;
	struct crtx_sdbus_listener *slist;
	
	
	slist = crtx_sdbus_get_default_listener(CRTX_SDBUS_TYPE_SYSTEM);
	
	r = sd_bus_message_new_method_call(slist->bus,
									&m,
									"org.freedesktop.Avahi",
									service->payload,
									"org.freedesktop.Avahi.EntryGroup",
									"Reset"
								);
	CRTX_RET_GEZ(r);
	
	r = sd_bus_call(slist->bus, m, -1, &error, &reply);
	if (r < 0) {
		ERROR("Failed to issue method call: %s %s\n", error.name, error.message);
		return r;
	}
	
	sd_bus_message_unref(m);
	
	return 0;
}

char crtx_avahi_service(struct crtx_event *event) {
	if (event->data.raw.type == 'p') {
		struct crtx_avahi_service *s = event->data.raw.pointer;
		
		switch (s->action) {
			case 'a': return crtx_avahi_publish_service(s);
			case 'r': return crtx_avahi_remove_service(s);
			default:
				ERROR("error, unknown crtx_avahi_service action %c\n", s->action);
				return -1;
		}
	} else
	if (event->data.dict) {
		int r;
		struct crtx_avahi_service s;
		char *action;
		
		
		r = crtx_get_value(event->data.dict, "interface", 'i', &s.interface, sizeof(s.interface));
		if (!r)
			s.interface = AVAHI_IF_UNSPEC;
		
		r = crtx_get_value(event->data.dict, "protocol", 'i', &s.protocol, sizeof(s.protocol));
		if (!r)
			s.protocol = AVAHI_PROTO_UNSPEC;
		
		r = crtx_get_value(event->data.dict, "flags", 'u', &s.flags, sizeof(s.flags));
		if (!r)
			s.flags = 0;
		
		r = crtx_get_value(event->data.dict, "name", 's', &s.name, sizeof(s.name));
		if (!r) {
			ERROR("crtx_avahi_service dict requires a \"name\" entry\n");
			return -1;
		}
		
		r = crtx_get_value(event->data.dict, "type", 's', &s.type, sizeof(s.type));
		if (!r) {
			ERROR("crtx_avahi_service dict requires a \"type\" entry, e.g., _http._tcp\n");
			return -1;
		}
		
		r = crtx_get_value(event->data.dict, "domain", 's', &s.domain, sizeof(s.domain));
		if (!r)
			s.domain = "";
		
		r = crtx_get_value(event->data.dict, "host", 's', &s.host, sizeof(s.host));
		if (!r)
			s.host = "";
		
		r = crtx_get_value(event->data.dict, "port", 'u', &s.port, sizeof(s.port));
		if (!r) {
			ERROR("crtx_avahi_service dict requires a \"port\" entry\n");
			return -1;
		}
		
		r = crtx_get_value(event->data.dict, "action", 's', &action, sizeof(action));
		if (!r) {
			ERROR("crtx_avahi_service dict requires a \"action\" entry, e.g., \"add\" or \"remove\"\n");
			return -1;
		}
		
		if (!strcmp(action, "add")) {
			crtx_avahi_publish_service(&s);
		} else if (!strcmp(action, "remove")) {
			crtx_avahi_remove_service(&s);
		} else {
			ERROR("invalid crtx_avahi_service action: %s\n", action);
			return -1;
		}
	}
	
	return 0;
}

static void cb_sdbus_event_release(struct crtx_event *event) {
	struct crtx_avahi_service *service;
	
	service = (struct crtx_avahi_service *) event->data.raw.pointer;
	sd_bus_message_unref(service->payload);
}


static char sdbus_to_avahi_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_avahi_listener *alist;
	sd_bus_message *m;
	struct crtx_avahi_service *service;
	struct crtx_event *avahi_event;
	int r;
	
	
	alist = (struct crtx_avahi_listener*) userdata;
	
	m = (sd_bus_message *) event->data.raw.pointer;
	
	service = calloc(1, sizeof(struct crtx_avahi_service));
	
	r = sd_bus_message_read_basic(m, 'i', &service->interface); CRTX_RET_GEZ(r)
	r = sd_bus_message_read_basic(m, 'i', &service->protocol); CRTX_RET_GEZ(r)
	r = sd_bus_message_read_basic(m, 's', &service->name); CRTX_RET_GEZ(r)
	r = sd_bus_message_read_basic(m, 's', &service->type); CRTX_RET_GEZ(r)
	r = sd_bus_message_read_basic(m, 's', &service->domain); CRTX_RET_GEZ(r)
	r = sd_bus_message_read_basic(m, 'u', &service->flags); CRTX_RET_GEZ(r)
	
	if (!strcmp(sd_bus_message_get_member(m), "ItemNew")) {
		service->action = 'a';
	} else
	if (!strcmp(sd_bus_message_get_member(m), "ItemRemove")) {
		service->action = 'r';
	} else {
		service->action = 0;
	}
	
	service->payload = m;
	sd_bus_message_ref(m);
	
	avahi_event = create_event("avahi/event", service, 0);
// 	avahi_event->data.raw.flags |= DIF_DATA_UNALLOCATED;
	avahi_event->cb_before_release = &cb_sdbus_event_release;
	
	add_event(alist->parent.graph, avahi_event);
	
	return 0;
}

static char init_service_browser(struct crtx_avahi_listener *alist) {
	int r;
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *m = NULL, *reply = NULL;
	
	
	r = sd_bus_message_new_method_call(alist->sdlist.bus,
									&m,
									"org.freedesktop.Avahi",
									"/",
									"org.freedesktop.Avahi.Server",
									"ServiceBrowserNew"
								);
	CRTX_RET_GEZ(r);
	
	r = sd_bus_message_append_basic(m, 'i', &alist->service_browser.interface); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 'i', &alist->service_browser.protocol); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', alist->service_browser.type); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 's', alist->service_browser.domain); CRTX_RET_GEZ(r)
	r = sd_bus_message_append_basic(m, 'u', &alist->service_browser.flags); CRTX_RET_GEZ(r)
	
	r = sd_bus_call(alist->sdlist.bus, m, -1, &error, &reply);
	if (r < 0) {
		ERROR("Failed to issue method call: %s %s\n", error.name, error.message);
		return r;
	}
	
	r = sd_bus_message_read(reply, "o", &alist->service_browser.payload); CRTX_RET_GEZ(r);
	
	alist->service_browser.payload = crtx_stracpy(alist->service_browser.payload, 0);
	
	sd_bus_message_unref(m);
	
	return 0;
}

struct crtx_listener_base *crtx_new_avahi_listener(void *options) {
	struct crtx_avahi_listener *alist;
	int ret;
	
	
	alist = (struct crtx_avahi_listener*) options;
	
	ret = crtx_open_sdbus(&alist->sdlist.bus, CRTX_SDBUS_TYPE_SYSTEM, 0);
	
	/*
	 * setup SD-BUS listener
	 */
	
	init_service_browser(alist);
	
	char *prefix = "type='signal',interface='org.freedesktop.Avahi.ServiceBrowser',member='ItemNew',path='";
	alist->sdbus_match[0].match_str = malloc(strlen(prefix)+strlen(alist->service_browser.payload)+2);
	sprintf(alist->sdbus_match[0].match_str, "%s%s'", prefix, (char*) alist->service_browser.payload);
	alist->sdbus_match[0].event_type = "avahi/sdbus_event";
	
	alist->sdbus_match[1].match_str = 0;
// 	alist->sdbus_match[1].match_str = "type='signal'";
// 	alist->sdbus_match[1].event_type = "avahi/sdbus_event";
	
	alist->sdbus_match[2].match_str = 0;
	
// 	memset(&sdlist, 0, sizeof(struct crtx_sdbus_listener));
	alist->sdlist.bus_type = CRTX_SDBUS_TYPE_SYSTEM;
	alist->sdlist.matches = alist->sdbus_match;
	
	alist->sdbase = create_listener("sdbus", &alist->sdlist);
	if (!alist->sdbase) {
		ERROR("create_listener(sdbus) failed\n");
		exit(1);
	}
	
	crtx_create_task(alist->sdbase->graph, 0, "sdbus_to_avahi_handler", sdbus_to_avahi_handler, alist);
	
	ret = crtx_start_listener(alist->sdbase);
	if (!ret) {
		ERROR("starting sdbus listener failed\n");
		return 0;
	}
	
	
// 	new_eventgraph(&alist->parent.graph, "avahi", 0);

// 	epl->parent.shutdown = &shutdown_avahi_listener;

	return &alist->parent;
}

void crtx_avahi_init() {
}

void crtx_avahi_finish() {
}


#ifdef CRTX_TEST

static char avahi_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_avahi_service *service;
	
	service = (struct crtx_avahi_service*) event->data.raw.pointer;
	
	printf("new service %s %s %s %u\n", service->name, service->type, service->host, service->port);
	
	return 0;
}

int avahi_main(int argc, char **argv) {
	struct crtx_avahi_listener alist;
	struct crtx_listener_base *lbase;
	char ret;
	struct crtx_avahi_service s;
	
	s.interface = AVAHI_IF_UNSPEC;
	s.protocol = AVAHI_PROTO_UNSPEC;
	s.flags = 0;
	s.name = "crtx";
	s.type = "_http._tcp";
	s.domain = "crtx_domain";
	s.host = "";
	s.port = 1234;
	
	crtx_avahi_publish_service(&s);
	
	
	
	memset(&alist, 0, sizeof(struct crtx_avahi_listener));
	
	alist.service_browser.interface = AVAHI_IF_UNSPEC;
	alist.service_browser.protocol = AVAHI_PROTO_UNSPEC;
	alist.service_browser.flags = 0;
	alist.service_browser.type = "_http._tcp";
	alist.service_browser.domain = "";
	
	lbase = create_listener("avahi", &alist);
	if (!lbase) {
		ERROR("create_listener(avahi) failed\n");
		exit(1);
	}
	
	crtx_create_task(lbase->graph, 0, "avahi_test", avahi_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting avahi listener failed\n");
		return 1;
	}
	
	
	crtx_loop();
	
	crtx_avahi_remove_service(&s);
	
	return 0;
}

CRTX_TEST_MAIN(avahi_main);
#endif
