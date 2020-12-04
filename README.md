
libcortex
=========

libcortex provides an event loop implementation and follows a "batteries included"
approach, ie. it already contains the specific code to monitor certain event sources.

The code for the different event sources are built into separate plugins that
are loaded on-demand during runtime. Hence, only the required plugins for a
use-case are loaded and only their dependencies have to be met.

The code of libcortex itself is licensed under the MIT license. However, many
plugins rely on third-party libraries with a different license. Please check
the effective license for your specific use-case.

Please note, not all plugins are actively used and many are not yet feature-complete.
Hence, check the corresponding source files if your use-case is already supported.

Actively used:
 * can
 * curl
 * fork
 * libnl
 * pipe
 * popen
 * sdbus
 * signals
 * udev
 * writequeue

Other modules:
 * avahi
 * evdev
 * fanotify
 * inotify
 * libvirt
 * netlink_ge
 * netlink_raw
 * nf_queue
 * nf_route_raw
 * pulseaudio
 * sip
 * uevents
 * v4l
 * xcb_randr


Example
-------

The following code will show information about a USB mass storage device whenever
udev detects that one was plugged in:

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crtx/core.h>
#include <crtx/udev.h>

// specific udev attributes we are interested in
char * usb_attrs[] = {
	"idVendor",
	"idProduct",
	"manufacturer",
	"product",
	"serial",
	0,
};

// the group of attributes we are interested in
struct crtx_udev_raw2dict_attr_req r2ds[] = {
	{ "usb", "usb_device", usb_attrs },
	{ 0 },
};

// the kind of udev events/devices we are interested in
char *udev_filters[] = {
	"block",
	"partition",
	0,
	0,
};

// callback function that is executed if a udev event is received
static char udev_event_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;
	
	// optional helper function that converts udev events into a key-value dictionary
	dict = crtx_udev_raw2dict((struct udev_device *) crtx_event_get_ptr(event), r2ds, 0);
	
	crtx_print_dict(dict);
	
	crtx_dict_unref(dict);
	
	return 0;
}

int main(int argc, char **argv) {
	struct crtx_udev_listener ulist;
	int ret;
	
	
	crtx_init();
	// optional helper function to gracefully handle standard process signals
	crtx_handle_std_signals();
	
	// initialize the control structure of this listener/event source
	memset(&ulist, 0, sizeof(struct crtx_udev_listener));
	
	ulist.sys_dev_filter = udev_filters;
	
	// initialize this listener
	ret = crtx_setup_listener("udev", &ulist);
	if (ret) {
		fprintf(stderr, "create_listener(udev) failed: %s\n", strerror(-ret));
		exit(1);
	}
	
	// register a callback function
	crtx_create_task(ulist.base.graph, 0, "udev_event", udev_event_handler, 0);
	
	// start listening for events
	ret = crtx_start_listener(&ulist.base);
	if (ret) {
		fprintf(stderr, "starting udev listener failed\n");
		return 1;
	}
	
	// start the actual event loop
	crtx_loop();
	
	crtx_finish();
	
	return 0;
}
```

Please note, some plugins also include example code (used to build a small test
application) at the end of the source file which can compiled using the
`make test` command.
