#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "core.h"
#include "evdev.h"

char *evdev_msg_etype[] = { EVDEV_MSG_ETYPE, 0 };

void send_event(struct crtx_evdev_listener *el, struct input_event *ev, char sync) {
	struct crtx_event *event;
	
	event = create_event(EVDEV_MSG_ETYPE, &ev, sizeof(struct input_event));
	event->data.raw.flags |= DIF_DATA_UNALLOCATED;
	
	reference_event_release(event);
	
	add_event(el->parent.graph, event);
	
	wait_on_event(event);
	
	dereference_event_release(event);
}

void *evdev_tmain(void *data) {
	struct crtx_evdev_listener *el;
	struct input_event ev;
	int ret;
	
	el = (struct crtx_evdev_listener*) data;
	
	while (!el->stop) {
		do {
			ret = libevdev_next_event(el->device, LIBEVDEV_READ_FLAG_NORMAL|LIBEVDEV_READ_FLAG_BLOCKING, &ev);
			if (ret == LIBEVDEV_READ_STATUS_SYNC) {
// 				printf("::::::::::::::::::::: dropped ::::::::::::::::::::::\n");
				while (ret == LIBEVDEV_READ_STATUS_SYNC) {
					send_event(el, &ev, 1);
					
					ret = libevdev_next_event(el->device, LIBEVDEV_READ_FLAG_SYNC, &ev);
				}
// 				printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
			} else if (ret == LIBEVDEV_READ_STATUS_SUCCESS) {
				send_event(el, &ev, 0);
			}
		} while (ret == LIBEVDEV_READ_STATUS_SYNC || ret == LIBEVDEV_READ_STATUS_SUCCESS || ret == -EAGAIN);
		
		if (ret != LIBEVDEV_READ_STATUS_SUCCESS && ret != -EAGAIN)
			fprintf(stderr, "Failed to handle events: %s\n", strerror(-ret));
	}
	
	return 0;
}

void crtx_free_evdev_listener(struct crtx_listener_base *data) {
	struct crtx_evdev_listener *el;
	
	el = (struct crtx_evdev_listener*) data;
	libevdev_free(el->device);
}

struct crtx_listener_base *crtx_new_evdev_listener(void *options) {
	struct crtx_evdev_listener *evdev;
	int ret;
	
	evdev = (struct crtx_evdev_listener *) options;
	
	evdev->fd = open(evdev->device_path, O_RDONLY);
	if (evdev->fd < 0) {
		fprintf(stderr, "failed to open device \"%s\": %s", evdev->device_path, strerror(errno));
		return 0;
	}
	
	ret = libevdev_new_from_fd(evdev->fd, &evdev->device);
	if (ret < 0) {
		fprintf(stderr, "initialization failed: %s\n", strerror(-ret));
		close(evdev->fd);
		return 0;
	}
	
// 	printf("Input device ID: bus %#x vendor %#x product %#x\n",
// 		   libevdev_get_id_bustype(evdev->device),
// 		   libevdev_get_id_vendor(evdev->device),
// 		   libevdev_get_id_product(evdev->device));
// 	printf("Evdev version: %x\n", libevdev_get_driver_version(evdev->device));
// 	printf("Input device name: \"%s\"\n", libevdev_get_name(evdev->device));
// 	printf("Phys location: %s\n", libevdev_get_phys(evdev->device));
// 	printf("Uniq identifier: %s\n", libevdev_get_uniq(evdev->device));
	
	DBG("new evdev listener on %s: %s\n", evdev->device_path, libevdev_get_name(evdev->device));
	
	evdev->parent.free = &crtx_free_evdev_listener;
	
	new_eventgraph(&evdev->parent.graph, 0, evdev_msg_etype);
// 	
	evdev->parent.thread = get_thread(evdev_tmain, evdev, 0);
// 	evdev->parent.thread->do_stop = &stop_thread;
	
	return &evdev->parent;
}

void crtx_evdev_init() {
}

void crtx_evdev_finish() {
}



#ifdef CRTX_TEST

static int
print_event(struct input_event *ev)
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
				ev->value);
	}
	return 0;
}

static char evdev_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct input_event *ev;
	
	ev = (struct input_event *) event->data.raw.pointer;
	
	print_event(ev);
	
	return 0;
}

int evdev_main(int argc, char **argv) {
	struct crtx_evdev_listener el;
	struct crtx_listener_base *lbase;
	char ret;
	
	if (argc < 2) {
		fprintf(stderr, "Usage: %s </dev/input/eventX>\n", argv[0]);
		return 1;
	}
	
	memset(&el, 0, sizeof(struct crtx_evdev_listener));
	
	el.device_path = argv[1];
	
	lbase = create_listener("evdev", &el);
	if (!lbase) {
		ERROR("create_listener(evdev) failed\n");
		exit(1);
	}
	
	crtx_create_task(lbase->graph, 0, "evdev_test", evdev_test_handler, 0);
	
	ret = crtx_start_listener(lbase);
	if (!ret) {
		ERROR("starting evdev listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	free_listener(lbase);
	
	return 0;
}

CRTX_TEST_MAIN(evdev_main);

#endif