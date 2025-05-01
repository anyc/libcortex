#ifndef _CRTX_MQTT_H
#define _CRTX_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
* Mario Kicherer (dev@kicherer.org) 2022
*
*/

#include <mosquitto.h>
#include "core.h"

#define CRTX_MQTT_EVT_CONNECT "mqtt.connect"
#define CRTX_MQTT_EVT_DISCONNECT "mqtt.disconnect"
#define CRTX_MQTT_EVT_MSG "mqtt.msg"
#define CRTX_MQTT_EVT_LOG "mqtt.log"

#define CRTX_MQTT_EVT_CONNECT_ID (1<<0)
#define CRTX_MQTT_EVT_DISCONNECT_ID (1<<1)
#define CRTX_MQTT_EVT_MSG_ID (1<<2)
#define CRTX_MQTT_EVT_LOG_ID (1<<3)

struct crtx_mqtt_listener {
	struct crtx_listener_base base;
	
	struct mosquitto *mosq;
	char *clientid;
	
	const char *host;
	unsigned int port;
	unsigned int keepalive;
	
	char clientid_allocated;
	char reconnect;
	
	void (*connect_cb)(struct mosquitto *mosq, void *obj, int rc);
	void (*disconnect_cb)(struct mosquitto *mosq, void *obj, int rc);
	void (*log_cb)(struct mosquitto *, void *userdata, int level, const char *msg);
	void (*msg_cb)(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
};

struct crtx_listener_base *crtx_setup_mqtt_listener(void *options);
void crtx_mqtt_clear_listener(struct crtx_mqtt_listener *lstnr);
CRTX_DECLARE_CALLOC_FUNCTION(mqtt)

void crtx_mqtt_init();
void crtx_mqtt_finish();

#ifdef __cplusplus
}
#endif

#endif
