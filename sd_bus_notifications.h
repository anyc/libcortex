
struct sd_bus_notifications_listener {
	struct listener parent;
};

void send_notification(char *icon, char *title, char *text, char **actions, char**chosen_action);
struct listener *new_sd_bus_notification_listener(void *options);