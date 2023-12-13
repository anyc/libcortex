#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <inttypes.h>

#include "intern.h"
#include "core.h"
#include "sdbus.h"
#include "threads.h"

struct crtx_sdbus_listener sdlist;

static int async_cb(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
	sd_bus_message **reply;
	
	reply = (sd_bus_message **) userdata;
	*reply = m;
	
	sd_bus_message_ref(m);
}

char startup_callback(struct crtx_event *event, void *userdata, void **sessiondata) {
	sd_bus_message *msg, *reply;
	const sd_bus_error *error;
	struct crtx_dict_item ditem;
	struct crtx_dict_item *object, *interface, *entry;
	int oidx, iidx, eidx;
	
	reply = 0;
	crtx_sdbus_get_objects_msg_async(&sdlist, (char*) userdata, &async_cb, &reply, 0);
	
	while (reply == 0 && !crtx_is_shutting_down()) {
		crtx_loop_onetime();
	}
	
	if (!reply)
		return 0;
	
	error = sd_bus_message_get_error(reply);
	if (error) {
		CRTX_ERROR("DBus error: %s %s\n", error->name, error->message);
		crtx_init_shutdown();
		return 0;
	}
	
	memset(&ditem, 0, sizeof(struct crtx_dict_item));
	
	crtx_sdbus_next_to_dict_item(reply, &ditem, 0);
	// crtx_print_dict_item(&ditem, 0);
	
	for (oidx=0; oidx < ditem.dict->signature_length; oidx++) {
		object = &ditem.dict->items[oidx];
		
		printf("%s\n", object->dict->items[0].string);
		
		for (iidx=0; iidx < object->dict->items[1].dict->signature_length; iidx++) {
			interface = &object->dict->items[1].dict->items[iidx];
			
			printf("  %s\n", interface->dict->items[0].string);
			
			for (eidx=0; eidx < interface->dict->items[1].dict->signature_length; eidx++) {
				entry = &interface->dict->items[1].dict->items[eidx];
				
				// printf("  %s\n", entry->dict->items[0].string);
				printf("    "); crtx_print_dict_item(entry, 1); printf("\n");
			}
		}
	}
	
	crtx_free_dict_item_data(&ditem);
	
	crtx_init_shutdown();
}

int main(int argc, char **argv) {
	int ret;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	memset(&sdlist, 0, sizeof(struct crtx_sdbus_listener));
	
	sdlist.bus_type = CRTX_SDBUS_TYPE_SYSTEM;
	sdlist.connection_signals = 1;
	
	ret = crtx_setup_listener("sdbus", &sdlist);
	if (ret) {
		CRTX_ERROR("create_listener(sdbus) failed: %s\n", strerror(-ret));
		exit(1);
	}
	
	ret = crtx_start_listener(&sdlist.base);
	if (ret) {
		CRTX_ERROR("starting sdbus listener failed\n");
		return 1;
	}
	
	if (!strcmp(argv[1], "tree")) {
		crtx_evloop_register_startup_callback(startup_callback, argv[2], 0);
		
		crtx_loop();
		
		ret = 0;
	} else {
		fprintf(stderr, "Usage: %s tree <service>\n", argv[0]);
		ret = 1;
	}
	
	crtx_finish();
	
	return ret;
}
