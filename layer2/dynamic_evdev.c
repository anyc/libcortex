
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
	
	if (dyn_evdev->on_free)
		dyn_evdev->on_free(dll);
	
	crtx_dll_unlink(&dyn_evdev->evdev_listeners, dll);
	
	el = (struct crtx_evdev_listener *) data;
	
// 	printf("free %s\n", el->device_path);
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
	
	// only accept devices with this flag in order to avoid legacy interfaces
	// that are incompatible with evdev (e.g., /dev/input/mouseX)
	if (!udev_device_get_property_value(dev, "LIBINPUT_DEVICE_GROUP"))
		return -1;
	
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
		
		if (dyn_evdev->on_free)
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
#include <sys/time.h>

#include "timer.h"

#define N_SLOTS 5
#define SLOT_TIME 200
struct counter {
	struct crtx_dll dll;
	struct crtx_evdev_listener lstnr;
	
	char *id_path;
	char *name;
// 	struct crtx_dict *device;
	unsigned char bucket;
	unsigned char n_valid_buckets;
	unsigned long n_events[N_SLOTS+1];
	struct timespec start[N_SLOTS+1];
};

#define TDIFF(start, end) ((end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec)/1000000)

// char * input_attrs[] = {
// 	"name",
// 	"phys",
// 	0,
// };
// 
// struct crtx_udev_raw2dict_attr_req r2ds[] = {
// 	{ "input", 0, input_attrs },
// 	{ 0 },
// };

static char dyn_evdev_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct counter *counter;
	struct crtx_dll *dll;
	struct crtx_dynamic_evdev_listener *dyn_evdev;
	int i, j;
// 	char *id_path, *name;
	unsigned long diff;
	struct input_event *ev;
	
// 	counter = (struct counter*) userdata + sizeof(struct crtx_evdev_listener *);
	
	if (event) {
		ev = (struct input_event *) event->data.pointer;
		
		// skip syn and misc events
		if (ev->type == EV_SYN || ev->type == EV_MSC) {
			return 0;
		}
	}
	
	dyn_evdev = (struct crtx_dynamic_evdev_listener*) userdata;
	i=0;
	for (dll = dyn_evdev->evdev_listeners; dll; dll=dll->next) {
// 		counter = (struct counter*) (dll->data + sizeof(struct crtx_evdev_listener *));
		counter = (struct counter*) dll;
		
// 		id_path = crtx_get_string(counter->device, "ID_PATH");
// 		name = crtx_get_string(counter->device, "name");
// 		name = crtx_dict_locate_string(counter->device, "input-/name"); // subsystem-device_type
		
// 		mvprintw(i, 0, "%s (%s %s): %d %.2f\n", counter->name, counter->id_path, counter->lstnr.device_path, counter->n_events, counter->n_events/(float)diff);
		printw("%s (%s %s): ", counter->name, counter->id_path, counter->lstnr.device_path);
		
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		diff = TDIFF(counter->start[counter->bucket], now);
		
		// only timer events trigger a bucket switch
		if (!event) {
			counter->bucket++;
			if (counter->bucket == N_SLOTS)
				counter->bucket = 0;
			
			if (counter->n_valid_buckets < N_SLOTS)
				counter->n_valid_buckets++;
			
			counter->n_events[counter->bucket] = 0;
// 			counter->start[counter->bucket] = (unsigned long) time(NULL);
			clock_gettime(CLOCK_MONOTONIC, &counter->start[counter->bucket]);
		}
		
// 		for (j=
		
		if (event && event->origin == &counter->lstnr.parent) {
			counter->n_events[counter->bucket]++;
			counter->n_events[N_SLOTS]++;
		}
		
		unsigned long n_events;
		float timespan;
		n_events = 0;
// 		length = 0;
		for (j=0;j<N_SLOTS;j++) {
			
// 			diff = (unsigned long) time(NULL) - counter->start[j];
// 			n_events += counter->n_events[j]/(float)diff;
			n_events += counter->n_events[j];
// 			length += diff;
			if (j == counter->bucket)
				printw(".", counter->n_events[j]);
			else
				printw(" ");
			printw("%d ", counter->n_events[j]);
			
// 			printw("%d %.2f ", counter->n_events[j], counter->n_events[j]/(float)diff);
		}
// 		n_events = n_events / 2;
		
// 		diff = (unsigned long) time(NULL) - counter->start[counter->bucket];
// 		clock_gettime(CLOCK_MONOTONIC_RAW, &now);
		diff = TDIFF(counter->start[counter->bucket], now);
		
		if (counter->n_valid_buckets > 0)
			timespan = (counter->n_valid_buckets-1)*SLOT_TIME + diff;
		else
			timespan = diff;
		printw("%d %.2f ", n_events, n_events/timespan*1000);
		
// 		diff = (unsigned long) time(NULL) - counter->start[N_SLOTS];
		diff = TDIFF(counter->start[N_SLOTS], now);
		printw("%d %.2f ", counter->n_events[N_SLOTS], counter->n_events[N_SLOTS]/(float)diff*1000);
		printw("\n");
		
// 		printf("%p %p\n", event->origin, &counter->lstnr);
		
		i++;
	}
	
	refresh();
	move(0,0);
	
	return 0;
}

struct crtx_dll *alloc_new_device_memory(struct udev_device *dev) {
// 	void *ptr;
	struct counter *counter;
	const char *id_path, *name;
	
	counter = (struct counter*) calloc(1, sizeof(struct counter));
	
	int i;
	for (i=0;i<=N_SLOTS;i++) {
// 		counter->start[i] = (unsigned long)time(NULL);
		clock_gettime(CLOCK_MONOTONIC, &counter->start[i]);
	}
	
	counter->bucket = 0;
	
	// we cannot use raw2dict as we cannot easily determine the parent device
	// with the required attributes
// 	counter->device = crtx_udev_raw2dict(dev, r2ds, 1);
	
	id_path = udev_device_get_property_value(dev, "ID_PATH");
	if (id_path) {
		counter->id_path = malloc(strlen(id_path)+1);
		strcpy(counter->id_path, id_path);
	}
	
	// From udev, we receive the evdev interface device for the input device.
	// The common name attribute is, however, stored in the input device.
	dev = udev_device_get_parent(dev);
	name = udev_device_get_sysattr_value(dev, "name");
	if (name) {
		counter->name = malloc(strlen(name)+1);
		strcpy(counter->name, name);
	}
	
	return &counter->dll;
}

void on_free_device(struct crtx_dll *dll) {
	struct counter *counter;
	
	counter = (struct counter *)dll;
	
	free(counter->id_path);
	free(counter->name);
// 	crtx_dict_unref(counter->device);
}

char timer_tick(struct crtx_event *event, void *userdata, void **sessiondata) {
	return dyn_evdev_test_handler(0, userdata, 0);
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
	
	struct crtx_timer_listener timer_lstnr;
	
	if (crtx_timer_get_listener(&timer_lstnr, 0, SLOT_TIME * 1000000, 0, SLOT_TIME * 1000000)) {
		crtx_create_task(timer_lstnr.parent.graph, 0, "timer_tick", timer_tick, &dyn_evdev);
	}
	
	crtx_start_listener(&timer_lstnr.parent);
	
	initscr ();
	curs_set (0);
	#endif
	
// 	crtx_create_task(lbase->graph, 0, "dyn_evdev_test", dyn_evdev_test_handler, 0);
	
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
