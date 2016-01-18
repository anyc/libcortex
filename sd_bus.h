
#include <systemd/sd-bus.h>

struct crtx_sdbus_listener {
	struct crtx_listener_base parent;
	
};

extern sd_bus *sd_bus_main_bus;

void sd_bus_add_signal_listener(sd_bus *bus, char *path, char *event_type);
void sd_bus_print_event_task(struct crtx_event *event, void *userdata);
void sd_bus_print_msg(sd_bus_message *m);
void crtx_sd_bus_init();
void crtx_sd_bus_finish();