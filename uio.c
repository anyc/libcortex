/*
 * Mario Kicherer (dev@kicherer.org) 2023
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <linux/limits.h>

#include "intern.h"
#include "core.h"
#include "uio.h"

#ifndef CRTX_TEST

struct uio_device *uio_devices = 0;
unsigned int n_uio_devices = 0;

static char fd_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_uio_listener *lstnr;
	struct crtx_event *nevent;
	uint32_t info;
	ssize_t nbytes;
	
	lstnr = (struct crtx_uio_listener *) userdata;
	
	if ((lstnr->flags & CRTX_UIO_FLAG_NO_READ) == 0) {
		nbytes = read(crtx_listener_get_fd(&lstnr->base), &info, sizeof(info));
		if (nbytes != (ssize_t)sizeof(info)) {
			CRTX_ERROR("read unexpected number of bytes from uio device %s: %zd != %zu\n", lstnr->uio_device->name, nbytes, sizeof(info));
			return 0;
		}
		
		nbytes = write(crtx_listener_get_fd(&lstnr->base), &info, sizeof(info));
		if (nbytes != (ssize_t)sizeof(info)) {
			CRTX_ERROR("write to uio device %s failed: %zd != %zu (%s)\n", lstnr->uio_device->name, nbytes, sizeof(info), strerror(errno));
			return 0;
		}
	}
	
	// create and send an empty event
	crtx_create_event(&nevent);
	crtx_add_event(lstnr->base.graph, nevent);
	
	return 0;
}

static char start_listener(struct crtx_listener_base *base_lstnr) {
	struct crtx_uio_listener *lstnr;
	char buf[PATH_MAX];
	int fd;
	
	
	lstnr = (struct crtx_uio_listener*) base_lstnr;
	
	snprintf(buf, sizeof(buf), "/dev/uio%d", lstnr->uio_device->index);
	fd = open(buf, O_RDWR);
	
	crtx_listener_add_fd(base_lstnr, fd, CRTX_EVLOOP_READ, 0,
						 &fd_event_handler, lstnr, 0, 0);
	
	return 0;
}

static void query_uios() {
	char buf[PATH_MAX];
	char buf2[PATH_MAX];
	struct uio_device *dev;
	ssize_t nbytes;
	int fd;
	unsigned int i;
	
	i = -1;
	while (1) {
		i += 1;
		
		// read the DT name of device $i
		snprintf(buf, sizeof(buf), "/sys/class/uio/uio%d/name", i);
		fd = open(buf, O_RDONLY);
		if (fd < 0) {
			break;
		}
		
		memset(buf2, 0, sizeof(buf));
		nbytes = read(fd, buf2, sizeof(buf));
		if (nbytes < 1) {
			CRTX_ERROR("could not read \"%s\"\n", buf);
			
			close(fd);
			
			continue;
		}
		buf2[nbytes-1] = 0;
		
		close(fd);
		
		n_uio_devices += 1;
		uio_devices = (struct uio_device*) realloc(uio_devices, sizeof(struct uio_device) * n_uio_devices);
		dev = &uio_devices[n_uio_devices-1];
		
		dev->name = strdup(buf2);
		dev->index = i;
	}
}

struct crtx_listener_base *crtx_setup_uio_listener(void *options) {
	struct crtx_uio_listener *lstnr;
	struct uio_device *dev;
	int i;
	
	
	lstnr = (struct crtx_uio_listener *) options;
	
	if (uio_devices == 0) {
		query_uios();
	}
	
	for (i = 0; i < n_uio_devices; i++) {
		dev = &uio_devices[i];
		
		if (!strcmp(dev->name, lstnr->name)) {
			lstnr->uio_device = dev;
			break;
		}
	}
	
	if (!lstnr->uio_device) {
		CRTX_ERROR("uio device \"%s\" not found\n", lstnr->name);
		return 0;
	}
	
	lstnr->base.start_listener = &start_listener;
	
	return &lstnr->base;
}

CRTX_DEFINE_ALLOC_FUNCTION(uio)

void crtx_uio_init() {
}

void crtx_uio_finish() {
}

#else /* CRTX_TEST */

static char uio_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	printf("interrupt\n");
	
	return 0;
}

struct crtx_uio_listener lstnr;

int uio_main(int argc, char **argv) {
	int ret;
	
	
	memset(&lstnr, 0, sizeof(struct crtx_uio_listener));
	
	lstnr.name = argv[1];
	
	ret = crtx_setup_listener("uio", &lstnr);
	if (ret) {
		CRTX_ERROR("create_listener(uio) failed: %s\n", strerror(-ret));
		exit(1);
	}
	
	crtx_create_task(lstnr.base.graph, 0, "uio_test", uio_test_handler, 0);
	
	ret = crtx_start_listener(&lstnr.base);
	if (ret) {
		CRTX_ERROR("starting uio listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	return 0;  
}

CRTX_TEST_MAIN(uio_main);

#endif
