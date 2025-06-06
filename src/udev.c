/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intern.h"
#include "core.h"
#include "udev.h"
#include "dict.h"

/* Note that USB strings are Unicode, UCS2 encoded, but the strings returned from
 * udev_device_get_sysattr_value() are UTF-8 encoded.
 */

/// convert udev_device into a crtx dictionary
struct crtx_dict *crtx_udev_raw2dict(struct udev_device *dev, struct crtx_udev_raw2dict_attr_req *r2ds, char make_persistent) {
	struct crtx_dict *dict;
	struct crtx_dict_item *di, *sdi;
	struct crtx_udev_raw2dict_attr_req *i;
	char **a;
	const char *subsystem, *device_type, *value;
	struct udev_device *new_dev;
	int flags;
	
	
	dict = crtx_init_dict(0, 0, 0);
	
	if (make_persistent)
		flags = CRTX_DIF_CREATE_DATA_COPY;
	else
		flags = CRTX_DIF_DONT_FREE_DATA;
	
	value = udev_device_get_syspath(dev);
	if (value) {
		di = crtx_dict_alloc_next_item(dict);
		crtx_fill_data_item(di, 's', "SYSPATH", value, strlen(value), flags);
	}
	
	// get only requested or all attributes
	if (r2ds && (r2ds->subsystem || r2ds->device_type)) {
		value = udev_device_get_devnode(dev);
		if (value) {
			di = crtx_dict_alloc_next_item(dict);
			crtx_fill_data_item(di, 's', "NODE", value, strlen(value), flags);
		}
		
		value = udev_device_get_subsystem(dev);
		if (value) {
			di = crtx_dict_alloc_next_item(dict);
			crtx_fill_data_item(di, 's', "SUBSYSTEM", value, strlen(value), flags);
		}
		
		value = udev_device_get_devtype(dev);
		if (value) {
			di = crtx_dict_alloc_next_item(dict);
			crtx_fill_data_item(di, 's', "DEVTYPE", value, strlen(value), flags);
		}
		
		value = udev_device_get_action(dev);
		if (value) {
			di = crtx_dict_alloc_next_item(dict);
			crtx_fill_data_item(di, 's', "ACTION", value, strlen(value), flags);
		} else {
			// if we query existing devices at startup, there is no action field
			di = crtx_dict_alloc_next_item(dict);
			crtx_fill_data_item(di, 's', "ACTION", "initial", strlen("initial"), flags);
		}
		
		i = r2ds;
		while (i->subsystem || i->device_type) {
			subsystem = udev_device_get_subsystem(dev);
			device_type = udev_device_get_devtype(dev);
			
			if (
				(i->subsystem && strcmp(subsystem, i->subsystem))
				|| (i->device_type && strcmp(device_type, i->device_type))
				)
			{
				new_dev = udev_device_get_parent_with_subsystem_devtype(dev, i->subsystem, i->device_type);
				if (new_dev)
					dev = new_dev;
				else
					i++;
				continue;
			}
			
			di = crtx_dict_alloc_next_item(dict);
			di->type = 'D';
			
			di->key = malloc( (i->subsystem?strlen(i->subsystem):0) + 1 + (i->device_type?strlen(i->device_type):0) + 1);
			sprintf(di->key, "%s-%s", i->subsystem?i->subsystem:"", i->device_type?i->device_type:"");
			di->flags |= CRTX_DIF_ALLOCATED_KEY;
			di->dict = crtx_init_dict(0, 0, 0);
			
			a = i->attributes;
			while (*a) {
				value = udev_device_get_sysattr_value(dev, *a);
				if (value) {
					sdi = crtx_dict_alloc_next_item(di->dict);
					crtx_fill_data_item(sdi, 's', *a, value, strlen(value), flags);
				} else {
					value = udev_device_get_property_value(dev, *a);
					if (value) {
						sdi = crtx_dict_alloc_next_item(di->dict);
						crtx_fill_data_item(sdi, 's', *a, value, strlen(value), flags);
					}
				}
				
				a++;
			}
 			
			i++;
		}
	} else {
		struct udev_list_entry *entry;
		const char *key;
		
		value = udev_device_get_action(dev);
		if (!value) {
			// if we query existing devices at startup, there is no action field
			di = crtx_dict_alloc_next_item(dict);
			crtx_fill_data_item(di, 's', "ACTION", "initial", strlen("initial"), flags);
		}
		
		udev_list_entry_foreach(entry, udev_device_get_properties_list_entry(dev)) {
			key = udev_list_entry_get_name(entry);
			value = udev_list_entry_get_value(entry);
			
			di = crtx_dict_alloc_next_item(dict);
			if (value)
				crtx_fill_data_item(di, 's', (char*) key, value, strlen(value), flags);
		}
	}
	
	//crtx_print_dict(dict);
	
	return dict;
}


void udev_event_before_release_cb(struct crtx_event *event, void *userdata) {
	struct udev_device *dev;
	
	dev = (struct udev_device *) crtx_event_get_ptr(event);
	udev_device_unref(dev);
}

void push_new_udev_event(struct crtx_udev_listener *ulist, struct udev_device *dev) {
	struct crtx_event *nevent;
	
	// size of struct udev_device is unknown
	crtx_create_event(&nevent);
	
	crtx_event_set_raw_data(nevent, 'p', dev, sizeof(struct udev_device*), CRTX_DIF_DONT_FREE_DATA);
	
	nevent->cb_before_release = &udev_event_before_release_cb;
	
	crtx_add_event(ulist->base.graph, nevent);
}

static int udev_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_udev_listener *ulist;
	struct udev_device *dev;
	
	
	ulist = (struct crtx_udev_listener *) userdata;
	
	dev = udev_monitor_receive_device(ulist->monitor);
	if (dev) {
		push_new_udev_event(ulist, dev);
	} else {
		CRTX_DBG("udev_monitor_receive_device returned no device\n");
	}
	
	return 0;
}

static void crtx_shutdown_udev_listener(struct crtx_listener_base *data) {
	struct crtx_udev_listener *ulist;
	
	ulist = (struct crtx_udev_listener*) data;
	
	udev_monitor_unref(ulist->monitor);
	udev_unref(ulist->udev);
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_udev_listener *ulist;
	
	ulist = (struct crtx_udev_listener *) listener;
	
	udev_monitor_enable_receiving(ulist->monitor);
	
	if (ulist->query_existing) {
		struct udev_enumerate *enumerate;
		struct udev_list_entry *devices, *dev_list_entry;
		const char *path;
		struct udev_device *dev;
		
		enumerate = udev_enumerate_new(ulist->udev);
		
		char **p = ulist->sys_dev_filter;
		while (p && (*p || *(p+1))) {
			if (*p)
				udev_enumerate_add_match_subsystem(enumerate, *p);
			if (*(p+1))
				udev_enumerate_add_match_property(enumerate, "DEVTYPE", *(p+1));
			p+=2;
		}
		
		udev_enumerate_scan_devices(enumerate);
		devices = udev_enumerate_get_list_entry(enumerate);
		
		udev_list_entry_foreach(dev_list_entry, devices) {
			path = udev_list_entry_get_name(dev_list_entry);
			dev = udev_device_new_from_syspath(ulist->udev, path);
			
			push_new_udev_event(ulist, dev);
		}
		
		udev_enumerate_unref(enumerate);
	}
	
	return 0;
}

struct crtx_listener_base *crtx_setup_udev_listener(void *options) {
	struct crtx_udev_listener *ulist;
	
	ulist = (struct crtx_udev_listener *) options;
	
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
	
	crtx_evloop_init_listener(&ulist->base,
						0, udev_monitor_get_fd(ulist->monitor),
						CRTX_EVLOOP_READ,
						0,
						&udev_fd_event_handler,
						ulist,
						0, 0
					);
	
	ulist->base.shutdown = &crtx_shutdown_udev_listener;
	
	ulist->base.start_listener = &start_listener;
	
	return &ulist->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(udev)

void crtx_udev_init() {
}

void crtx_udev_finish() {
}
