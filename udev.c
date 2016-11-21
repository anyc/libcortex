
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "udev.h"

char *udev_msg_etype[] = { UDEV_MSG_ETYPE, 0 };

/* Note that USB strings are Unicode, UCS2 encoded, but the strings returned from
 * udev_device_get_sysattr_value() are UTF-8 encoded.
 */

struct crtx_dict *crtx_udev_raw2dict(struct crtx_event *event, struct crtx_udev_raw2dict_attr_req *r2ds) {
	struct crtx_dict *dict;
	struct crtx_dict_item *di, *sdi;
	struct crtx_udev_raw2dict_attr_req *i;
	char **a;
	const char *subsystem, *device_type, *value;
	struct udev_device *dev, *new_dev;
	
	dev = (struct udev_device *) event->data.raw.pointer;
	
	dict = crtx_init_dict(0, 0, 0);
	
	value = udev_device_get_syspath(dev);
	if (value) {
		di = crtx_alloc_item(dict);
		crtx_fill_data_item(di, 's', "SYSPATH", value, strlen(value), DIF_DATA_UNALLOCATED);
	}
	
	// get only requested or all attributes
	if (r2ds && (r2ds->subsystem || r2ds->device_type)) {
		value = udev_device_get_devnode(dev);
		if (value) {
			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 's', "NODE", value, strlen(value), DIF_DATA_UNALLOCATED);
		}
		
		value = udev_device_get_subsystem(dev);
		if (value) {
			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 's', "SUBSYSTEM", value, strlen(value), DIF_DATA_UNALLOCATED);
		}
		
		value = udev_device_get_devtype(dev);
		if (value) {
			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 's', "DEVTYPE", value, strlen(value), DIF_DATA_UNALLOCATED);
		}
		
		value = udev_device_get_action(dev);
		if (value) {
			di = crtx_alloc_item(dict);
			crtx_fill_data_item(di, 's', "ACTION", value, strlen(value), DIF_DATA_UNALLOCATED);
		}
		
		i = r2ds;
		while (i->subsystem || i->device_type) {
			subsystem = udev_device_get_subsystem(dev);
			device_type = udev_device_get_devtype(dev);
			
			if (strcmp(subsystem, i->subsystem) || strcmp(device_type, i->device_type)) {
				new_dev = udev_device_get_parent_with_subsystem_devtype(dev, i->subsystem, i->device_type);
				
				if (new_dev)
					dev = new_dev;
				else
					continue;
			}
			
			di = crtx_alloc_item(dict);
			di->type = 'D';
			
			di->key = malloc( (i->subsystem?strlen(i->subsystem):0) + 1 + (i->device_type?strlen(i->device_type):0) + 1);
			sprintf(di->key, "%s-%s", i->subsystem?i->subsystem:"", i->device_type?i->device_type:"");
			di->flags |= DIF_KEY_ALLOCATED;
			di->ds = crtx_init_dict(0, 0, 0);
			
			a = i->attributes;
			while (*a) {
				value = udev_device_get_sysattr_value(dev, *a);
				if (value) {
					sdi = crtx_alloc_item(di->ds);
					crtx_fill_data_item(sdi, 's', *a, value, strlen(value), DIF_DATA_UNALLOCATED);
				}
				
				a++;
			}
			
			i++;
		}
	} else {
		struct udev_list_entry *entry;
		const char *key;
		
		udev_list_entry_foreach(entry, udev_device_get_properties_list_entry(dev)) {
			key = udev_list_entry_get_name(entry);
			value = udev_list_entry_get_value(entry);
			
			di = crtx_alloc_item(dict);
			if (value)
				crtx_fill_data_item(di, 's', key, value, strlen(value), DIF_DATA_UNALLOCATED);
		}
	}
	
	return dict;
}


void udev_event_before_release_cb(struct crtx_event *event) {
	struct udev_device *dev;
	
	dev = (struct udev_device *) event->data.raw.pointer;
	udev_device_unref(dev);
}


static char udev_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_event_loop_payload *payload;
	struct crtx_udev_listener *ulist;
	struct udev_device *dev;
	
	
	payload = (struct crtx_event_loop_payload*) event->data.raw.pointer;
	
	ulist = (struct crtx_udev_listener *) payload->data;
	
	dev = udev_monitor_receive_device(ulist->monitor);
	if (dev) {
		struct crtx_event *nevent;
		
		// size of struct udev_device is unknown
		nevent = create_event(UDEV_MSG_ETYPE, dev, sizeof(struct udev_device*));
		nevent->data.raw.flags |= DIF_DATA_UNALLOCATED;
		
		nevent->cb_before_release = &udev_event_before_release_cb;
// 		reference_event_release(nevent);
		
		add_event(ulist->parent.graph, nevent);
		
// 		wait_on_event(nevent);
		
// 		dereference_event_release(nevent);
		
// 		udev_device_unref(dev);
	} else {
		printf("No Device from receive_device(). An error occured.\n");
	}
	
	return 0;
}

void crtx_free_udev_listener(struct crtx_listener_base *data) {
	struct crtx_udev_listener *ulist;
	
	ulist = (struct crtx_udev_listener*) data;
	
	udev_monitor_unref(ulist->monitor);
	udev_unref(ulist->udev);
}

struct crtx_listener_base *crtx_new_udev_listener(void *options) {
	struct crtx_udev_listener *ulist;
	
	ulist = (struct crtx_udev_listener *) options;
	
// 	DBG("new evdev listener on %s: %s\n", evdev->device_path, libevdev_get_name(evdev->device));
	
	/* Create the udev object */
	ulist->udev = udev_new();
	if (!ulist->udev) {
		fprintf(stderr, "libudev initialization failed\n");
		return 0;
	}
	
	ulist->monitor = udev_monitor_new_from_netlink(ulist->udev, "udev");
	
	char **p = ulist->sys_dev_filter;
	while (p && (*p || *(p+1))) {
		udev_monitor_filter_add_match_subsystem_devtype(ulist->monitor, *p, *(p+1));
		p+=2;
	}
	
	ulist->parent.el_payload.fd = udev_monitor_get_fd(ulist->monitor);
	ulist->parent.el_payload.data = ulist;
	ulist->parent.el_payload.event_handler = &udev_fd_event_handler;
	ulist->parent.el_payload.event_handler_name = "udev fd handler";
	
	ulist->parent.free = &crtx_free_udev_listener;
	new_eventgraph(&ulist->parent.graph, "udev", udev_msg_etype);
	
	udev_monitor_enable_receiving(ulist->monitor);
	
	return &ulist->parent;
}

void crtx_udev_init() {
}

void crtx_udev_finish() {
}



#ifdef CRTX_TEST

char * usb_attrs[] = {
	"idVendor",
	"idProduct",
	"manufacturer",
	"product",
	"serial",
	0,
};

struct crtx_udev_raw2dict_attr_req r2ds[] = {
	{ "usb", "usb_device", usb_attrs },
	{ 0 },
};

char *test_filters[] = {
	"block",
	"partition",
	0,
	0,
};

static char udev_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct udev_device *dev, *usb_dev;
	struct crtx_dict *dict;
	
// 	dev = (struct udev_device *) event->data.raw.pointer;
	
	dict = crtx_udev_raw2dict(event, r2ds);
	
	crtx_print_dict(dict);
	
	crtx_free_dict(dict);
	
	return 0;
}

int udev_main(int argc, char **argv) {
	struct crtx_udev_listener ulist;
	struct crtx_listener_base *lbase;
	char ret;
	
// 	crtx_handle_std_signals();
	
	crtx_root->no_threads = 1;
	
	memset(&ulist, 0, sizeof(struct crtx_udev_listener));
	
	ulist.sys_dev_filter = test_filters;
	
	lbase = create_listener("udev", &ulist);
	if (!lbase) {
		ERROR("create_listener(udev) failed\n");
		exit(1);
	}
	
	crtx_create_task(lbase->graph, 0, "udev_test", udev_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting udev listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	free_listener(lbase);
	
	return 0;  
}

CRTX_TEST_MAIN(udev_main);

#endif
