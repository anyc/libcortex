#ifndef _CRTX_SD_BUS_NOTIFICATIONS_H
#define _CRTX_SD_BUS_NOTIFICATIONS_H

/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

// struct crtx_sd_bus_notification_listener {
// 	struct crtx_listener_base parent;
// };

// void send_notification(char *icon, char *title, char *text, char **actions, char**chosen_action);
// struct crtx_listener_base *crtx_new_sd_bus_notification_listener(void *options);

void crtx_sd_bus_notifications_init();
void crtx_sd_bus_notifications_finish();

#endif
