
#include <systemd/sd-bus.h>

extern sd_bus *sd_bus_main_bus;

void sd_bus_add_listener(sd_bus *bus, char *path, char *event_type);
void sd_bus_print_event_task(struct event *event, void *userdata);
void sd_bus_print_msg(sd_bus_message *m);
void sd_bus_init();
void sd_bus_finish();