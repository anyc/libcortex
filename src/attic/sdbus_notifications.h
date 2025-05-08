#ifndef _CRTX_SD_BUS_NOTIFICATIONS_H
#define _CRTX_SD_BUS_NOTIFICATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

// struct crtx_sd_bus_notification_listener {
// 	struct crtx_listener_base base;
// };

// void send_notification(char *icon, char *title, char *text, char **actions, char**chosen_action);
// struct crtx_listener_base *crtx_setup_sd_bus_notification_listener(void *options);

void crtx_sdbus_notifications_init();
void crtx_sdbus_notifications_finish();

#ifdef __cplusplus
}
#endif

#endif
