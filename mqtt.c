/*
 * Mario Kicherer (dev@kicherer.org) 2022
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <mosquitto.h>

#include "core.h"
#include "timer.h"
#include "mqtt.h"

#ifndef CRTX_TEST

void crtx_mqtt_default_connect_callback(struct mosquitto *mosq, void *userdata, int rc) {
	struct crtx_mqtt_listener *mlstnr;
	int rv;
	
	
	mlstnr = (struct crtx_mqtt_listener *) userdata;
	
	if (rc)
		CRTX_DBG("MQTT connect callback error: %s (%d)\n", mosquitto_strerror(rc), rc);

#if 0
	struct crtx_event *event;
	
	rv = crtx_create_event(&event);
	if (rv) {
		CRTX_ERROR("crtx_create_event failed: %s\n", strerror(rv));
		return;
	}
	
	event->type = CRTX_MQTT_EVT_CONNECT_ID;
	event->description = CRTX_MQTT_EVT_CONNECT;
	
	crtx_fill_data_item(&event->data, 'i', 0, rc, sizeof(rc), 0);
	
	crtx_add_event(mlstnr->base.graph, event);
#else
	rv = crtx_push_new_event(&mlstnr->base, 0, CRTX_MQTT_EVT_CONNECT_ID, CRTX_MQTT_EVT_CONNECT, 'i', 0, rc, sizeof(rc), 0);
	if (rv) {
		CRTX_ERROR("crtx_push_new_event failed: %s\n", strerror(rv));
	}
#endif
}

void crtx_mqtt_default_disconnect_callback(struct mosquitto *mosq, void *userdata, int rc) {
	struct crtx_mqtt_listener *mlstnr;
	int rv;
	
	
	mlstnr = (struct crtx_mqtt_listener *) userdata;
	
	if (rc)
		CRTX_ERROR("MQTT disconnect callback error: %s (%d, fd %d)\n", mosquitto_strerror(rc), rc, mlstnr->base.evloop_fd.fd);

#if 0
	struct crtx_event *event;
	
	rv = crtx_create_event(&event);
	if (rv) {
		CRTX_ERROR("crtx_create_event failed: %s\n", strerror(rv));
		return;
	}
	
	event->type = CRTX_MQTT_EVT_DISCONNECT_ID;
	event->description = CRTX_MQTT_EVT_DISCONNECT;
	
	crtx_fill_data_item(&event->data, 'i', 0, rc, sizeof(rc), 0);
	
	crtx_add_event(mlstnr->base.graph, event);
#else
	rv = crtx_push_new_event(&mlstnr->base, 0, CRTX_MQTT_EVT_DISCONNECT_ID, CRTX_MQTT_EVT_DISCONNECT, 'i', 0, rc, sizeof(rc), 0);
	if (rv) {
		CRTX_ERROR("crtx_push_new_event failed: %s\n", strerror(rv));
	}
#endif
}

void crtx_mqtt_default_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *msg) {
	struct crtx_mqtt_listener *mlstnr;
	int rv;
	
	
	mlstnr = (struct crtx_mqtt_listener *) userdata;
	
#if 0
	struct crtx_event *event;
	
	rv = crtx_create_event(&event);
	if (rv) {
		CRTX_ERROR("crtx_create_event failed: %s\n", strerror(rv));
		return;
	}
	
	event->type = CRTX_MQTT_EVT_LOG_ID;
	event->description = CRTX_MQTT_EVT_LOG;
	
	crtx_event_set_dict(event, "is",
						"level", level, sizeof(level), 0,
						"msg", msg, (size_t) 0, CRTX_DIF_CREATE_DATA_COPY
					   );
	
	crtx_add_event(mlstnr->base.graph, event);
#else
	rv = crtx_push_new_event(&mlstnr->base, 0, CRTX_MQTT_EVT_LOG_ID, CRTX_MQTT_EVT_LOG,
						'D', "is",
						"level", level, sizeof(level), 0,
						"msg", msg, (size_t) 0, CRTX_DIF_CREATE_DATA_COPY
					);
	if (rv) {
		CRTX_ERROR("crtx_push_new_event failed: %s\n", strerror(rv));
	}
#endif
}

void crtx_mqtt_default_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
	struct crtx_mqtt_listener *mlstnr;
	int rv;
	
	
	mlstnr = (struct crtx_mqtt_listener *) userdata;
	
#if 0
	struct crtx_event *event;
	
	rv = crtx_create_event(&event);
	if (rv) {
		CRTX_ERROR("crtx_create_event failed: %s\n", strerror(rv));
		return;
	}
	
	event->type = CRTX_MQTT_EVT_MSG_ID;
	event->description = CRTX_MQTT_EVT_MSG;
	
	crtx_fill_data_item(&event->data, 's', message->topic, message->payload, message->payloadlen, CRTX_DIF_CREATE_KEY_COPY | CRTX_DIF_CREATE_DATA_COPY);
	
	crtx_add_event(mlstnr->base.graph, event);
#else
	rv = crtx_push_new_event(&mlstnr->base, 0, CRTX_MQTT_EVT_MSG_ID, CRTX_MQTT_EVT_MSG, 's', message->topic, message->payload, message->payloadlen, CRTX_DIF_CREATE_KEY_COPY | CRTX_DIF_CREATE_DATA_COPY);
	if (rv) {
		CRTX_ERROR("crtx_push_new_event failed: %s\n", strerror(rv));
	}
#endif
}

static char mqtt_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_mqtt_listener *mlstnr;
	struct crtx_evloop_callback *el_cb;
	int rv, flags;
	
	
	el_cb = (struct crtx_evloop_callback *) event->data.pointer;
	
	mlstnr = (struct crtx_mqtt_listener *) userdata;
	
	if (crtx_verbosity >= CRTX_VLEVEL_DBG) {
		CRTX_DBG("mqtt event "); crtx_event_flags2str(stdout, el_cb->triggered_flags); printf("\n");
	}
	
	if (el_cb->triggered_flags & EVLOOP_READ) {
		rv = mosquitto_loop_read(mlstnr->mosq, 1);
		if (rv == MOSQ_ERR_CONN_LOST) {
			crtx_lstnr_handle_fd_closed(&mlstnr->base);
			crtx_stop_listener(&mlstnr->base);
			
			if (mlstnr->reconnect)
				crtx_timer_retry_listener_os(&mlstnr->base, 1, 0);
		}
	}
	if (el_cb->triggered_flags & EVLOOP_WRITE)
		mosquitto_loop_write(mlstnr->mosq, 1);
	
	mosquitto_loop_misc(mlstnr->mosq);
	
	// the docs suggest to call mosquitto_loop_misc() once a second
	
	crtx_evloop_set_timeout_rel(el_cb, 1000000);
	
	flags = EVLOOP_TIMEOUT | EVLOOP_READ | (mosquitto_want_write(mlstnr->mosq)?EVLOOP_WRITE:0);
	if (flags != el_cb->crtx_event_flags) {
		el_cb->crtx_event_flags = flags;
		crtx_evloop_enable_cb(el_cb);
	}
	
	return 0;
}

static char start_listener(struct crtx_listener_base *listener) {
	int rv, evtypes;
	struct crtx_mqtt_listener *mlstnr;
	
	
	mlstnr = (struct crtx_mqtt_listener *) listener;
	
	CRTX_DBG("mosquitto_connect(%p, %s, %u, %u)\n", mlstnr->mosq, mlstnr->host, mlstnr->port, mlstnr->keepalive);
	
	rv = mosquitto_connect(mlstnr->mosq, mlstnr->host, mlstnr->port, mlstnr->keepalive);
	if (rv) {
		CRTX_ERROR("mosquitto_connect() failed: %s\n", mosquitto_strerror(rv));
		
		if (mlstnr->reconnect)
			crtx_timer_retry_listener_os(&mlstnr->base, 1, 0);
		
		return rv;
	}
	
	evtypes = EVLOOP_TIMEOUT | EVLOOP_READ;
	if (mosquitto_want_write(mlstnr->mosq)) {
		evtypes |= EVLOOP_WRITE;
	}
	
	crtx_evloop_init_listener(&mlstnr->base,
							mosquitto_socket(mlstnr->mosq),
							evtypes,
							0,
							&mqtt_fd_event_handler,
							mlstnr,
							0, 0
						);
	
	return 0;
}

static void shutdown_listener(struct crtx_listener_base *listener) {
	struct crtx_mqtt_listener *mlstnr;
	
	mlstnr = (struct crtx_mqtt_listener *) listener;
	
	mosquitto_destroy(mlstnr->mosq);
	if (mlstnr->clientid_allocated)
		free(mlstnr->clientid);
	
	return;
}

struct crtx_listener_base *crtx_setup_mqtt_listener(void *options) {
	struct crtx_mqtt_listener *mlstnr;
	
	
	mlstnr = (struct crtx_mqtt_listener *) options;
	
	mlstnr->base.start_listener = &start_listener;
	mlstnr->base.shutdown = &shutdown_listener;
	
	if (!mlstnr->clientid) {
		mlstnr->clientid_allocated = 1;
		mlstnr->clientid = malloc(32);
		snprintf(mlstnr->clientid, 32, "crtx-mqtt-%d", getpid());
	}
	
	if (!mlstnr->keepalive)
		mlstnr->keepalive = 60;
	
	mlstnr->mosq = mosquitto_new(mlstnr->clientid, true, mlstnr);
	
	if (!mlstnr->connect_cb)
		mlstnr->connect_cb = &crtx_mqtt_default_connect_callback;
	mosquitto_connect_callback_set(mlstnr->mosq, mlstnr->connect_cb);
	
	if (!mlstnr->disconnect_cb)
		mlstnr->disconnect_cb = &crtx_mqtt_default_disconnect_callback;
	mosquitto_disconnect_callback_set(mlstnr->mosq, mlstnr->disconnect_cb);
	
	if (!mlstnr->log_cb)
		mlstnr->log_cb = &crtx_mqtt_default_log_callback;
	mosquitto_log_callback_set(mlstnr->mosq, mlstnr->log_cb);
	
	if (!mlstnr->msg_cb)
		mlstnr->msg_cb = &crtx_mqtt_default_message_callback;
	mosquitto_message_callback_set(mlstnr->mosq, mlstnr->msg_cb);
	
	return &mlstnr->base;
}

void crtx_mqtt_clear_lstnr(struct crtx_mqtt_listener *lstnr) {
	memset(lstnr, 0, sizeof(struct crtx_mqtt_listener));
}

CRTX_DEFINE_ALLOC_FUNCTION(mqtt)

void crtx_mqtt_init() {
	mosquitto_lib_init();
}

void crtx_mqtt_finish() {
	mosquitto_lib_cleanup();
}

#else /* CRTX_TEST */

char * topic = 0;

static char mqtt_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_mqtt_listener *mlist;
	int rv;
	
	
	mlist = (struct crtx_mqtt_listener *) userdata;
	
	switch (event->type) {
		case CRTX_MQTT_EVT_MSG_ID:
			CRTX_INFO("RX: %s = %s\n", event->data.key, event->data.string);
			break;
		case CRTX_MQTT_EVT_LOG_ID:
			if (event->data.type == 'D') {
				crtx_printf(crtx_get_item(event->data.dict, "level")->int32, "LOG: %s\n", crtx_get_string(event->data.dict, "msg"));
			} else {
				CRTX_ERROR("unexpected event type %c\n", event->data.type);
			}
			break;
		case CRTX_MQTT_EVT_CONNECT_ID:
			CRTX_INFO("connected (%d)\n", event->data.int32);
			
			rv = mosquitto_subscribe(mlist->mosq, NULL, topic, 0);
			if (rv) {
				CRTX_ERROR("mosquitto_subscribe() failed: %s\n", mosquitto_strerror(rv));
				return rv;
			}
			break;
		case CRTX_MQTT_EVT_DISCONNECT_ID:
			CRTX_INFO("disconnected: \"%s\" (%d)\n", mosquitto_strerror(event->data.int32), event->data.int32);
			break;
	}
	
	return 0;
}

// void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
// 	printf("received message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);
// }

int main(int argc, char **argv) {
	struct crtx_mqtt_listener mlist;
	int rv;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	crtx_mqtt_clear_lstnr(&mlist);
	
	mlist.host = "localhost";
	mlist.port = 1883;
// 	mlist.msg_cb = &message_callback;
	mlist.reconnect = 1;
	
	rv = crtx_setup_listener("mqtt", &mlist);
	if (rv) {
		CRTX_ERROR("create_listener(mqtt) failed\n");
		return 0;
	}
	
	if (argc > 3)
		mosquitto_username_pw_set(mlist.mosq, argv[2], argv[3]);
	if (argc > 5)
		mosquitto_tls_psk_set(mlist.mosq, argv[4], argv[5], NULL);
	
	crtx_create_task(mlist.base.graph, 0, "mqtt_event", &mqtt_event_handler, &mlist);
	
	rv = crtx_start_listener(&mlist.base);
	if (rv) {
		CRTX_ERROR("starting mqtt listener failed\n");
		return 0;
	}
	
	if (argc < 2)
		topic = "#";
	else
		topic = argv[1];
	
	crtx_loop();
	
	crtx_finish();
	
	return 0;
}

#endif
