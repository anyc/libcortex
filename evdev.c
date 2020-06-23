/*
 * Mario Kicherer (dev@kicherer.org) 2016
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "intern.h"
#include "core.h"
#include "evdev.h"

// char *evdev_msg_etype[] = { EVDEV_MSG_ETYPE, 0 };

void evdev_event_before_release_cb(struct crtx_event *event, void *userdata) {
	free(crtx_event_get_ptr(event));
// 	crtx_event_set_data(event, 0, 0, 0);
	crtx_event_set_raw_data(event, 'p', 0, 0, 0);
}

static void send_event(struct crtx_evdev_listener *el, struct input_event *ev, char sync) {
	struct crtx_event *event;
	
	struct input_event *new_ev;
	new_ev = (struct input_event *) malloc(sizeof(struct input_event));
	memcpy(new_ev, ev, sizeof(struct input_event));
	
	crtx_create_event(&event);
	crtx_event_set_raw_data(event, 'p', new_ev, sizeof(struct input_event), 0);
// 	event->data.flags |= CRTX_DIF_DONT_FREE_DATA;
	event->cb_before_release = &evdev_event_before_release_cb;
	
// 	reference_event_release(event);
	
	crtx_add_event(el->base.graph, event);
	
// 	wait_on_event(event);
	
// 	dereference_event_release(event);
}

// void *evdev_tmain(void *data) {
// 	struct crtx_evdev_listener *el;
// 	struct input_event ev;
// 	int ret;
// 	
// 	el = (struct crtx_evdev_listener*) data;
// 	
// 	while (!el->stop) {
// 		do {
// 			ret = libevdev_next_event(el->device, LIBEVDEV_READ_FLAG_NORMAL|LIBEVDEV_READ_FLAG_BLOCKING, &ev);
// 			if (ret == LIBEVDEV_READ_STATUS_SYNC) {
// // 				printf("::::::::::::::::::::: dropped ::::::::::::::::::::::\n");
// 				while (ret == LIBEVDEV_READ_STATUS_SYNC) {
// 					send_event(el, &ev, 1);
// 					
// 					ret = libevdev_next_event(el->device, LIBEVDEV_READ_FLAG_SYNC, &ev);
// 				}
// // 				printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
// 			} else if (ret == LIBEVDEV_READ_STATUS_SUCCESS) {
// 				send_event(el, &ev, 0);
// 			}
// 		} while (ret == LIBEVDEV_READ_STATUS_SYNC || ret == LIBEVDEV_READ_STATUS_SUCCESS || ret == -EAGAIN);
// 		
// 		if (ret != LIBEVDEV_READ_STATUS_SUCCESS && ret != -EAGAIN) {
// 			fprintf(stderr, "Failed to handle events on %s: %s\n", el->device_path, strerror(-ret));
// 			break;
// 		}
// 	}
// 	
// 	return 0;
// }

static char evdev_fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
// 	struct crtx_evloop_callback *el_cb;
	struct crtx_evdev_listener *evdev;
	struct input_event ev;
	int ret;
	
	
// 	el_cb = (struct crtx_evloop_callback*) event->data.pointer;
	evdev = (struct crtx_evdev_listener *) userdata;
	
	do {
		ret = libevdev_next_event(evdev->device, LIBEVDEV_READ_FLAG_NORMAL, &ev);
		switch (ret) {
			case LIBEVDEV_READ_STATUS_SYNC:
				do {
					send_event(evdev, &ev, 1);
					
					ret = libevdev_next_event(evdev->device, LIBEVDEV_READ_FLAG_SYNC, &ev);
				} while (ret == LIBEVDEV_READ_STATUS_SYNC);
				break;
			case LIBEVDEV_READ_STATUS_SUCCESS:
				send_event(evdev, &ev, 0);
				break;
			default:
				break;
		}
	} while (ret == LIBEVDEV_READ_STATUS_SUCCESS);
	
	if (ret != LIBEVDEV_READ_STATUS_SUCCESS && ret != -EAGAIN) {
		ERROR("evdev: failed to handle events of %s: %s\n", evdev->device_path, strerror(-ret));
	}
	
	return 0;
}

void crtx_shutdown_evdev_listener(struct crtx_listener_base *data) {
	struct crtx_evdev_listener *el;
	
	el = (struct crtx_evdev_listener*) data;
	libevdev_free(el->device);
}

struct crtx_listener_base *crtx_new_evdev_listener(void *options) {
	struct crtx_evdev_listener *evdev;
	int ret;
	
	evdev = (struct crtx_evdev_listener *) options;
	
	evdev->fd = open(evdev->device_path, O_RDONLY | O_NONBLOCK);
	if (evdev->fd < 0) {
		fprintf(stderr, "failed to open device \"%s\": %s\n", evdev->device_path, strerror(errno));
		return 0;
	}
	
	ret = libevdev_new_from_fd(evdev->fd, &evdev->device);
	if (ret < 0) {
		fprintf(stderr, "initialization failed (%s fd %d): %s\n", evdev->device_path, evdev->fd, strerror(-ret));
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
	
	evdev->base.shutdown = &crtx_shutdown_evdev_listener;
	
// 	new_eventgraph(&evdev->base.graph, 0, evdev_msg_etype);
	
// 	evdev->base.thread = get_thread(evdev_tmain, evdev, 0);
// 	evdev->base.thread->do_stop = &stop_thread;
	
// 	evdev->base.evloop_fd.fd = evdev->fd;
// 	evdev->base.evloop_fd.data = evdev;
// 	evdev->base.evloop_fd.crtx_event_flags = EVLOOP_READ;
// 	evdev->base.evloop_fd.event_handler = &evdev_fd_event_handler;
// 	evdev->base.evloop_fd.event_handler_name = "evdev fd handler";
	crtx_evloop_init_listener(&evdev->base,
						evdev->fd,
						EVLOOP_READ,
						0,
						&evdev_fd_event_handler,
						evdev,
						0, 0
					);
	
	return &evdev->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(evdev)

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
	
	ev = (struct input_event *) crtx_event_get_ptr(event);
	
	print_event(ev);
	
	return 0;
}

int evdev_main(int argc, char **argv) {
	struct crtx_evdev_listener el;
	char ret;
	
	
	if (argc < 2) {
		fprintf(stderr, "Usage: %s </dev/input/eventX>\n", argv[0]);
		return 1;
	}
	
	memset(&el, 0, sizeof(struct crtx_evdev_listener));
	
	el.device_path = argv[1];
	
	ret = crtx_create_listener("evdev", &el);
	if (ret) {
		ERROR("create_listener(evdev) failed: %s\n", strerror(-ret));
		exit(1);
	}
	
	crtx_create_task(el.base.graph, 0, "evdev_test", evdev_test_handler, 0);
	
	ret = crtx_start_listener(&el.base);
	if (ret) {
		ERROR("starting evdev listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	crtx_free_listener(&el.base);
	
	return 0;
}

CRTX_TEST_MAIN(evdev_main);

#endif
