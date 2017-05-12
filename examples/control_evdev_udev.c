
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "udev.h"
#include "evdev.h"

struct crtx_udev_listener ulist;
// struct crtx_evdev_listener *evdev_listeners;
// unsigned char n_evdev_listeners = 0;
struct crtx_dll *evdev_listeners = 0;

char *udev_input_filters[] = {
	"input", 0,
	0, 0,
};

static int print_event(struct input_event *ev)
{
	if (ev->type == EV_SYN) {
		printf("Event: time %ld.%06ld, ++++++++++++++++++++ %s +++++++++++++++\n",
			   ev->time.tv_sec,
		 ev->time.tv_usec,
		 libevdev_event_type_get_name(ev->type));
	} else {
		printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n",
				ev->time.tv_sec,
				ev->time.tv_usec,
				ev->type,
				libevdev_event_type_get_name(ev->type),
				ev->code,
				libevdev_event_code_get_name(ev->type, ev->code),
				ev->value
			);
	}
	return 0;
}

static char evdev_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct input_event *ev;
	
	ev = (struct input_event *) event->data.pointer;
	
	print_event(ev);
	
	return 0;
}

static void on_free_evdev_listener_data(struct crtx_listener_base *data, void *userdata) {
	struct crtx_evdev_listener *el;
	struct crtx_dll *dll = (struct crtx_dll*) userdata;
	
	crtx_dll_unlink(&evdev_listeners, dll);
	
	el = (struct crtx_evdev_listener *) dll->data;
	
	free(el->device_path);
// 	free_listener(&el->parent);
// 	free(el);
	free(dll);
}

int add_evdev_listener(const char *devpath) {
	struct crtx_listener_base *lbase;
	struct crtx_evdev_listener *el;
	char ret;
	struct crtx_dll *dll;
	
	
	el = (struct crtx_evdev_listener *) calloc(1, sizeof(struct crtx_evdev_listener));
	dll = crtx_dll_append_new(&evdev_listeners, el);
	
	el->device_path = crtx_stracpy(devpath, 0);
	el->parent.free = on_free_evdev_listener_data;
	el->parent.free_userdata = dll;
	
	lbase = create_listener("evdev", el);
	if (!lbase) {
		ERROR("create_listener(evdev) failed\n");
		
		free(el->device_path);
		free(el);
		crtx_dll_unlink(&evdev_listeners, dll);
		free(dll);
		
		return 1;
	}
	
	crtx_create_task(lbase->graph, 0, "evdev_test", evdev_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting evdev listener failed\n");
		
		crtx_free_listener(lbase);
		free(el->device_path);
		free(el);
		crtx_dll_unlink(&evdev_listeners, dll);
		free(dll);
		
		return 1;
	}
	
	return 0;
}

static char udev_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct udev_device *dev;
	const char *devpath;
	
// 	struct crtx_dict *dict;
// 	dict = crtx_udev_raw2dict(event, 0);
// 	crtx_print_dict(dict);
// 	crtx_free_dict(dict);
	
	dev = (struct udev_device *) event->data.pointer;
	
	if (!strcmp(udev_device_get_action(dev), "add")) {
		devpath = udev_device_get_property_value(dev, "DEVNAME");
		if (devpath)
			add_evdev_listener(devpath);
	}
	
	return 0;
}

char init() {
	struct crtx_listener_base *lbase;
	char ret;
	
	memset(&ulist, 0, sizeof(struct crtx_udev_listener));
	
	ulist.sys_dev_filter = udev_input_filters;
	
	lbase = create_listener("udev", &ulist);
	if (!lbase) {
		ERROR("create_listener(udev) failed\n");
		return 0;
	}
	
	crtx_create_task(lbase->graph, 0, "udev_test", udev_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting udev listener failed\n");
		return 0;
	}
	
	return 1;  
}

void finish() {
	struct crtx_dll *eit, *eitn;
	struct crtx_evdev_listener *el;
	
	crtx_free_listener((struct crtx_listener_base *) &ulist);
	
	for (eit=evdev_listeners;eit;eit=eitn) {
		eitn = eit->next;
		el = eit->data;
		
		crtx_free_listener(&el->parent);
		free(el);
	}
}
