
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "dynamic_evdev.h"


static char *udev_input_filters[] = {
	"input", 0,
	0, 0,
};

static void on_free_evdev_listener_data(struct crtx_listener_base *data, void *userdata) {
	struct crtx_evdev_listener *el;
	struct crtx_dll *dll; // = (struct crtx_dll*) userdata;
	struct crtx_dynamic_evdev_listener *dyn_evdev;
	
	dyn_evdev = (struct crtx_dynamic_evdev_listener *) userdata;
	
	for (dll=dyn_evdev->evdev_listeners; dll && dll->data != data; dll=dll->next) {}
	if (!dll) {
		ERROR("evdev_listener (%p) not found in list\n", data);
		return;
	}
	
	dyn_evdev->on_free(dll);
	
	crtx_dll_unlink(&dyn_evdev->evdev_listeners, dll);
	
	el = (struct crtx_evdev_listener *) data;
	
	free(el->device_path);
// 	free_listener(&el->parent);
// 	free(el);
	free(dll);
}

int add_evdev_listener(struct crtx_dynamic_evdev_listener *dyn_evdev, struct udev_device *dev) {
	struct crtx_listener_base *lbase;
	struct crtx_evdev_listener *el;
	char ret;
	struct crtx_dll *dll;
	const char *devpath;
// 	struct crtx_event *event;
// 	struct crtx_dict *dict;
	
	
// 	el = (struct crtx_evdev_listener *) calloc(1, sizeof(struct crtx_evdev_listener));
// 	dll = crtx_dll_append_new(&dyn_evdev->evdev_listeners, el);
	
	devpath = udev_device_get_property_value(dev, "DEVNAME");
	if (!devpath)
		return -1;
	
	if (dyn_evdev->alloc_new) {
		dll = dyn_evdev->alloc_new(dev);
		crtx_dll_append(&dyn_evdev->evdev_listeners, dll);
		dll->data = dll->payload;
	} else {
		dll = (struct crtx_dll*) calloc(1, sizeof(struct crtx_evdev_listener)+sizeof(struct crtx_dll));
		crtx_dll_append(&dyn_evdev->evdev_listeners, dll);
		dll->data = dll->payload;
	}
	
	el = (struct crtx_evdev_listener *) dll->data;
	
	el->device_path = crtx_stracpy(devpath, 0);
	el->parent.free = on_free_evdev_listener_data;
	el->parent.free_userdata = dyn_evdev;
	
	lbase = create_listener("evdev", el);
	if (!lbase) {
		ERROR("create_listener(evdev) failed\n");
		
		dyn_evdev->on_free(dll);
		
		free(el->device_path);
// 		free(el);
		crtx_dll_unlink(&dyn_evdev->evdev_listeners, dll);
		free(dll);
		
		return 1;
	}
	
	crtx_create_task(lbase->graph, 0, "dyn_evdev_handler", dyn_evdev->handler, dyn_evdev);
	
// 		printf("dll %p %p %p\n", dll, el, lbase->graph->listener);
	
// 	event = crtx_create_event("NEW_DEVICE");
// 	dict = crtx_create_dict("",
// 			"
// 			);
// 	crtx_event_set_dict_data(event, dict, 0);
	
	// 	reference_event_release(event);
	
// 	crtx_add_event(el->parent.graph, event);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting evdev listener failed\n");
		
		crtx_free_listener(lbase);
// 		free(el->device_path);
// 		free(el);
// 		crtx_dll_unlink(&dyn_evdev->evdev_listeners, dll);
// 		free(dll);
		
		return 1;
	}
	
	return 0;
}

static char udev_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct udev_device *dev;
// 	const char *devpath;
	struct crtx_dynamic_evdev_listener *dyn_evdev;
	const char *action;
	
	dyn_evdev = (struct crtx_dynamic_evdev_listener *) userdata;
	
// 	dev = (struct udev_device *) event->data.pointer;
	dev = (struct udev_device *) crtx_event_get_ptr(event);
	
	action = udev_device_get_action(dev);
	
	if (!action || !strcmp(action, "add")) {
// 		devpath = udev_device_get_property_value(dev, "DEVNAME");
// 		if (devpath)
// 			add_evdev_listener(dyn_evdev, devpath);
		add_evdev_listener(dyn_evdev, dev);
	}
	
	return 0;
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_dynamic_evdev_listener *dyn_evdev;
	char ret;
	
	dyn_evdev = (struct crtx_dynamic_evdev_listener *) listener;
	
	ret = crtx_start_listener(&dyn_evdev->udev_lstnr.parent);
	if (!ret) {
		ERROR("starting udev listener failed\n");
		return 0;
	}
	
	return 1;
}

static void shutdown_listener(struct crtx_listener_base *data) {
	struct crtx_dll *eit, *eitn;
	struct crtx_evdev_listener *el;
	struct crtx_dynamic_evdev_listener *dyn_evdev;
	
	dyn_evdev = (struct crtx_dynamic_evdev_listener *) data;
	
	crtx_free_listener((struct crtx_listener_base *) &dyn_evdev->udev_lstnr);
	
	for (eit=dyn_evdev->evdev_listeners;eit;eit=eitn) {
		eitn = eit->next;
		el = eit->data;
		
// 		free(el->device_path);
		crtx_free_listener(&el->parent);
// 		free(el);
// 		free(eit);
	}
}

struct crtx_listener_base *crtx_new_dynamic_evdev_listener(void *options) {
	struct crtx_listener_base *udev_lbase;
	struct crtx_dynamic_evdev_listener *dyn_evdev;
// 	char ret;
	
	dyn_evdev = (struct crtx_dynamic_evdev_listener *) options;
	
	memset(&dyn_evdev->udev_lstnr, 0, sizeof(struct crtx_udev_listener));
	
	dyn_evdev->udev_lstnr.sys_dev_filter = udev_input_filters;
	dyn_evdev->udev_lstnr.query_existing = 1;
	
	udev_lbase = create_listener("udev", &dyn_evdev->udev_lstnr);
	if (!udev_lbase) {
		ERROR("create_listener(udev) failed\n");
		return 0;
	}
	
	crtx_create_task(udev_lbase->graph, 0, "udev_test", udev_test_handler, dyn_evdev);
	
	dyn_evdev->parent.shutdown = &shutdown_listener;
	dyn_evdev->parent.start_listener = &start_listener;
	
	return &dyn_evdev->parent;  
}

void crtx_dynamic_evdev_init() {
	struct crtx_listener_repository *lrepo;
	
	lrepo = crtx_get_new_listener_repo_entry();
	
	lrepo->id = "dynamic_evdev";
	lrepo->create = &crtx_new_dynamic_evdev_listener;
}

void crtx_dynamic_evdev_finish() {
}


#ifdef CRTX_TEST

#ifdef WITH_NCURSES
#include <ncurses.h>

struct counter {
	struct crtx_dll dll;
	struct crtx_evdev_listener lstnr;
	
	char *id_path;
	unsigned long n_events;
};

static char dyn_evdev_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct counter *counter;
	struct crtx_dll *dll;
	struct crtx_dynamic_evdev_listener *dyn_evdev;
	int i;
	
// 	counter = (struct counter*) userdata + sizeof(struct crtx_evdev_listener *);
	
	dyn_evdev = (struct crtx_dynamic_evdev_listener*) userdata;
	i=0;
	for (dll = dyn_evdev->evdev_listeners; dll; dll=dll->next) {
// 		counter = (struct counter*) (dll->data + sizeof(struct crtx_evdev_listener *));
		counter = (struct counter*) dll;
		mvprintw (i, 0, "%s: %d\n", counter->id_path, counter->n_events);
		
// 		printf("%p %p\n", event->origin, &counter->lstnr);
		if (event->origin == &counter->lstnr.parent)
			counter->n_events++;
		
		i++;
	}
	
	refresh ();
	
	return 0;
}

struct crtx_dll *alloc_new_device_memory(struct udev_device *dev) {
// 	void *ptr;
	struct counter *counter;
	const char *id_path;
	
	counter = (struct counter*) calloc(1, sizeof(struct counter));
	
	id_path = udev_device_get_property_value(dev, "ID_PATH");
	if (id_path) {
		counter->id_path = malloc(strlen(id_path)+1);
		strcpy(counter->id_path, id_path);
	}
	
	return &counter->dll;
}

void on_free_device(struct crtx_dll *dll) {
	struct counter *counter;
	
	counter = (struct counter *)dll;
	
	free(counter->id_path);
}

#else

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

static char dyn_evdev_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct input_event *ev;
	
	ev = (struct input_event *) event->data.pointer;
	
	print_event(ev);
	
	return 0;
}

#endif

int dynamic_evdev_main(int argc, char **argv) {
	struct crtx_listener_base *lbase;
	struct crtx_dynamic_evdev_listener dyn_evdev;
	char ret;
	
	memset(&dyn_evdev, 0, sizeof(struct crtx_dynamic_evdev_listener));
	
	dyn_evdev.handler = &dyn_evdev_test_handler;
	
	lbase = create_listener("dynamic_evdev", &dyn_evdev);
	if (!lbase) {
		ERROR("create_listener(dynamic_evdev) failed\n");
		return 0;
	}
	
	#ifdef WITH_NCURSES
	dyn_evdev.alloc_new = alloc_new_device_memory;
	dyn_evdev.on_free = on_free_device;
	
	initscr ();
	curs_set (0);
	#endif
	
	crtx_create_task(lbase->graph, 0, "dyn_evdev_test", dyn_evdev_test_handler, 0);
	
// 	crtx_create_task(dyn_evdev.udev_lstnr.parent.graph, 0, "on_new_device", on_new_device, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting dynamic_evdev listener failed\n");
		return 0;
	}
	
	crtx_loop();
	
	#ifdef WITH_NCURSES
	endwin();
	#endif
	
	crtx_free_listener(lbase);
	
	return 0;
}

CRTX_TEST_MAIN(dynamic_evdev_main);

#endif
