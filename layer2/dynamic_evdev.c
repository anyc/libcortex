/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WITH_NCURSES
#include <rrd.h>
#endif

#include "intern.h"
#include "core.h"
#include "dynamic_evdev.h"


static char *udev_input_filters[] = {
	"input", 0,
	0, 0,
};

static void on_free_evdev_listener_data(struct crtx_listener_base *data, void *userdata) {
	struct crtx_evdev_listener *el;
	struct crtx_dll *dll;
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
	
	free(el->device_path);
	free(dll);
}

int add_evdev_listener(struct crtx_dynamic_evdev_listener *dyn_evdev, struct udev_device *dev) {
// 	struct crtx_listener_base *lbase;
	struct crtx_evdev_listener *el;
	char ret;
	struct crtx_dll *dll;
	const char *devpath;
	
	
	devpath = udev_device_get_property_value(dev, "DEVNAME");
	if (!devpath) {
		INFO("ignoring device due to missing device path\n");
		return -1;
	}
	
	// only accept devices with this flag in order to avoid legacy interfaces
	// that are incompatible with evdev (e.g., /dev/input/mouseX)
	if (!udev_device_get_property_value(dev, "LIBINPUT_DEVICE_GROUP")) {
		INFO("ignoring %s due to missing libinput flags\n", devpath);
		return -1;
	}
	
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
	
// 	lbase = create_listener("evdev", el);
	ret = crtx_create_listener("evdev", el);
	if (ret) {
		ERROR("create_listener(evdev) failed\n");
		
		if (dyn_evdev->on_free)
			dyn_evdev->on_free(dll);
		
		free(el->device_path);
// 		free(el);
		crtx_dll_unlink(&dyn_evdev->evdev_listeners, dll);
		free(dll);
		
		return 1;
	}
	
	crtx_create_task(el->parent.graph, 0, "dyn_evdev_handler", dyn_evdev->handler, dyn_evdev->handler_userdata);
	
// 		printf("dll %p %p %p\n", dll, el, lbase->graph->listener);
	
// 	event = crtx_create_event("NEW_DEVICE");
// 	dict = crtx_create_dict("",
// 			"
// 			);
// 	crtx_event_set_dict_data(event, dict, 0);
	
	// 	reference_event_release(event);
	
// 	crtx_add_event(el->parent.graph, event);
	
	ret = crtx_start_listener(&el->parent);
	if (ret) {
		ERROR("starting evdev listener failed\n");
		
		crtx_free_listener(&el->parent);
		
		return 1;
	}
	
	DBG("new evdev device: %s\n", devpath);
	
	return 0;
}

static char udev_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct udev_device *dev;
//  	const char *devpath;
	struct crtx_dynamic_evdev_listener *dyn_evdev;
	const char *action;
	
	dyn_evdev = (struct crtx_dynamic_evdev_listener *) userdata;
	
	dev = (struct udev_device *) crtx_event_get_ptr(event);
	
	action = udev_device_get_action(dev);
	
	if (!action || !strcmp(action, "add")) {
//  		devpath = udev_device_get_property_value(dev, "DEVNAME");
		
		add_evdev_listener(dyn_evdev, dev);
	}
	
	return 0;
}

static char start_listener(struct crtx_listener_base *listener) {
	struct crtx_dynamic_evdev_listener *dyn_evdev;
	char ret;
	
	dyn_evdev = (struct crtx_dynamic_evdev_listener *) listener;
	
	ret = crtx_start_listener(&dyn_evdev->udev_lstnr.parent);
	if (ret) {
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
		
		crtx_free_listener(&el->parent);
	}
}

struct crtx_listener_base *crtx_new_dynamic_evdev_listener(void *options) {
// 	struct crtx_listener_base *udev_lbase;
	struct crtx_dynamic_evdev_listener *dyn_evdev;
	int ret;
	
	
	dyn_evdev = (struct crtx_dynamic_evdev_listener *) options;
	
	memset(&dyn_evdev->udev_lstnr, 0, sizeof(struct crtx_udev_listener));
	
	dyn_evdev->udev_lstnr.sys_dev_filter = udev_input_filters;
	dyn_evdev->udev_lstnr.query_existing = 1;
	
// 	udev_lbase = create_listener("udev", &dyn_evdev->udev_lstnr);
	ret = crtx_create_listener("udev", &dyn_evdev->udev_lstnr);
	if (ret) {
		ERROR("create_listener(udev) failed\n");
		return 0;
	}
	
	crtx_create_task(dyn_evdev->parent.graph, 0, "udev_test", udev_test_handler, dyn_evdev);
	
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

struct interval {
	unsigned int n_buckets;
	unsigned int n_ticks;
	
	unsigned int remaining_ticks;
	unsigned char n_valid_buckets;
	unsigned char bucket;
	
	unsigned long *n_events;
	struct timespec *start;
};

#define SLOT_TIME 200
struct counter {
	struct crtx_dll dll;
	struct crtx_evdev_listener lstnr;
	
	char *id_path;
	char *name;
	
	unsigned n_ivs;
	struct interval *ivs;
};

#define TDIFF(start, end) ((end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec)/1000000)

static char dyn_evdev_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct counter *counter;
	struct crtx_dll *dll;
	struct crtx_dynamic_evdev_listener *dyn_evdev;
	int i, j;
	unsigned long diff;
	struct input_event *ev;
	unsigned long n_events;
	float timespan;
	
	if (event) {
		ev = (struct input_event *) event->data.pointer;
		
		// skip syn and misc events
		if (ev->type == EV_SYN || ev->type == EV_MSC) {
			return 0;
		}
	}
	
	clear();
// 	getmaxyx(stdscr, mrow, mcol);
	
	dyn_evdev = (struct crtx_dynamic_evdev_listener*) userdata;
	for (dll = dyn_evdev->evdev_listeners; dll; dll=dll->next) {
		struct timespec now;
		struct interval *iv;
		
		counter = (struct counter*) dll;
		
		printw("%s (%s %s): \t", counter->name, counter->id_path, counter->lstnr.device_path);
		
		clock_gettime(CLOCK_MONOTONIC, &now);
		
		for (i=0; i < counter->n_ivs; i++) {
			iv = &counter->ivs[i];
			
			if (event && event->origin == &counter->lstnr.parent) {
				iv->n_events[iv->bucket]++;
			}
		
			n_events = 0;
			for (j=0;j<iv->n_buckets;j++) {
				n_events += iv->n_events[j];
			}
			
			diff = TDIFF(iv->start[iv->bucket], now);
			timespan = (iv->n_valid_buckets-1)*iv->n_ticks*SLOT_TIME + diff;
			
			printw("%d %.2f ", n_events, n_events/timespan*1000);
			
// 			diff = TDIFF(counter->start[N_SLOTS], now);
// 			printw("%d %.2f ", counter->n_events[N_SLOTS], counter->n_events[N_SLOTS]/(float)diff*1000);
// 			printw("\n");
			
			// only timer events trigger a bucket switch
			if (!event && iv->n_buckets > 1) {
				iv->remaining_ticks--;
				if (iv->remaining_ticks == 0) {
					iv->remaining_ticks = iv->n_ticks;
					
					if (i==1 && !strcmp("/dev/input/event11", counter->lstnr.device_path)) {
						char ustr[16];
						snprintf(ustr, 15, "N:%lu", iv->n_events[iv->bucket]);
						
						char *updateparams[] = {
							"rrdupdate",
							"evdev.rrd",
							ustr,
							NULL
						};
						printf("ustr %s\n", ustr);
						
						optind = opterr = 0;
						rrd_clear_error();
						rrd_update(3, updateparams);
					}
					
					iv->bucket++;
					if (iv->bucket == iv->n_buckets)
						iv->bucket = 0;
					
					if (iv->n_valid_buckets < iv->n_buckets)
						iv->n_valid_buckets++;
					
					iv->n_events[iv->bucket] = 0;
					clock_gettime(CLOCK_MONOTONIC, &iv->start[iv->bucket]);
				}
			}
		}
		printw("\n");
	}
	
	refresh();
	move(0,0);
	
	return 0;
}

struct interval ivs[] = {
	{ 5, 1 },
	{ 5, 5 },
	{ 1, 0 },
	{ 0 },
};

struct crtx_dll *alloc_new_device_memory(struct udev_device *dev) {
	struct counter *counter;
	const char *id_path, *name;
	struct interval *iv;
	int i, j;
	
	counter = (struct counter*) calloc(1, sizeof(struct counter));
	
// 	counter->n_ivs = 3;
	counter->ivs = (struct interval*) malloc(sizeof(ivs));
	memcpy(counter->ivs, ivs, sizeof(ivs));
// 	counter->ivs = ivs;
	
// 	for (i=0;i<counter->n_ivs;i++) {
	i=0;
	while (1) {
		iv = &counter->ivs[i];
		if (iv->n_buckets == 0)
			break;
		
// 		if (i < counter->n_ivs-1)
// 			iv->n_buckets = 5;
// 		else
// 			iv->n_buckets = 1;
// 		
// 		if (i == 
// 		iv->slot_time = 200;
		
		iv->start = (struct timespec*) malloc(sizeof(struct timespec)*iv->n_buckets);
		iv->n_events = (unsigned long*) calloc(1, sizeof(unsigned long)*iv->n_buckets);
		
		for (j=0;j<iv->n_buckets;j++) {
			clock_gettime(CLOCK_MONOTONIC, &iv->start[j]);
		}
		
		iv->remaining_ticks = iv->n_ticks;
		iv->bucket = 0;
		iv->n_valid_buckets = 1;
		
		i++;
	}
	counter->n_ivs = i;
	
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
	int i;
	
	counter = (struct counter *)dll;
	
	for (i=0;i<counter->n_ivs;i++) {
		free(counter->ivs[i].start);
		free(counter->ivs[i].n_events);
	}
	free(counter->ivs);
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
// 	struct crtx_listener_base *lbase;
	struct crtx_dynamic_evdev_listener dyn_evdev;
	char ret;
	
	
	memset(&dyn_evdev, 0, sizeof(struct crtx_dynamic_evdev_listener));
	
	dyn_evdev.handler = &dyn_evdev_test_handler;
	dyn_evdev.handler_userdata = &dyn_evdev;
	
// 	lbase = create_listener("dynamic_evdev", &dyn_evdev);
	ret = crtx_create_listener("dynamic_evdev", &dyn_evdev);
	if (ret) {
		ERROR("create_listener(dynamic_evdev) failed\n");
		return 0;
	}
	
	#ifdef WITH_NCURSES
	int err;
	
	dyn_evdev.alloc_new = alloc_new_device_memory;
	dyn_evdev.on_free = on_free_device;
	
	struct crtx_timer_listener timer_lstnr;
	
	if (crtx_timer_get_listener(&timer_lstnr, 0, SLOT_TIME * 1000000, 0, SLOT_TIME * 1000000)) {
		crtx_create_task(timer_lstnr.parent.graph, 0, "timer_tick", timer_tick, &dyn_evdev);
	}
	
	crtx_start_listener(&timer_lstnr.parent);
	
	char *createparams[] = {
		"rrdcreate",
		"evdev.rrd",
		"DS:evdev:COUNTER:2:0:U",
// 		"DS:myval:GAUGE:600:0:U",
		"RRA:AVERAGE:0.5:1:576",
		NULL
	};
	
	optind = 0;
	opterr = 0;
	rrd_clear_error();
	err = rrd_create(4, createparams);
	
	
	printf("%s\n", rrd_strerror(err));
	
	initscr ();
	curs_set (0);
	#endif
	
	ret = crtx_start_listener(&dyn_evdev.parent);
	if (!ret) {
		ERROR("starting dynamic_evdev listener failed\n");
		return 0;
	}
	
	crtx_loop();
	
	#ifdef WITH_NCURSES
	endwin();
	#endif
	
	crtx_free_listener(&dyn_evdev.parent);
	
	return 0;
}

CRTX_TEST_MAIN(dynamic_evdev_main);

#endif
