
// struct crtx_sd_bus_notification_listener {
// 	struct crtx_listener_base parent;
// };

// void send_notification(char *icon, char *title, char *text, char **actions, char**chosen_action);
// struct crtx_listener_base *crtx_new_sd_bus_notification_listener(void *options);

void crtx_sdbus_notification_init();
void crtx_sdbus_notification_finish();
