
libcortex
=========

libcortex provides an event loop implementation and follows a "batteries included"
approach, i.e. it already contains the specific code to monitor certain event sources.

The code for the different event sources is split into separate plugins that
are loaded on-demand during runtime. Hence, only the required plugins for a
use-case are loaded and only their dependencies have to be met.

The code of libcortex itself is licensed under the MIT license. However, many
plugins rely on third-party libraries with a different license. Please check
the effective license for your specific use-case.

Please note, not all plugins are actively used and many are not yet feature-complete.
Hence, check the corresponding source files if your use-case is already supported.

Actively used:
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
 * can
 * evdev
 * fanotify
 * inotify
 * libvirt
 * mqtt
 * netlink_ge
 * netlink_raw
 * nf_queue
 * nf_route_raw
 * pulseaudio
 * sd_journal
 * sip
 * uevents
 * v4l
 * xcb_randr

Why do I need an event loop?
----------------------------

A regular process of an application is single-threaded. If such an application is
event-driven, i.e. waits on events from external sources and processes them,
the `main()` function of the application usually does some initialization and then
calls a special function from a corresponding library that will not return until
the application should terminate. The name of this function usually contains
"loop" or "process" in some form. This function starts a so-called event loop.
Instead of polling the event source, an event loop implementation usually instructs
the operating system kernel to notify the application if an event occurred and
then the application sleeps until it is notified by the kernel. When the application
is awake again, the event loop will execute a callback function previously
registered by the application for the corresponding type of event.

Every library that provides access to a certain event source usually provides
such an interface. However, if the application should process events from more
than one event source, this approach is not feasible anymore as we cannot execute
multiple loop functions with one thread. One solution is to start a separate thread
for every event source that each will call the loop function of the corresponding
library. However, using multiple threads might cause additional source code
complexity due to the required thread synchronization and finding synchronization
bugs in multi-threaded applications can become a time-intensive effort if the
bugs occur only in rare circumstances.

If events do not have to be processed in parallel to achieve minimal latency,
another approach can be used: instead of starting an own event loop for every
event source, a generic event loop is started and the single event sources are
registered with this loop. Libcortex implements such a generic event loop but
also provides a simple interface to start listening to some common event sources
in a (Linux) operating system.

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
	
	// optional helper function that converts udev events into a key=value dictionary
	dict = crtx_udev_raw2dict((struct udev_device *) crtx_event_get_ptr(event), r2ds, 0);
	
	// print the key=value tuples that show information like the corresponding device file in /dev
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
