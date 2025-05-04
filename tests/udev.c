#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "udev.h"

char * usb_attrs[] = {
	"idVendor",
	"idProduct",
	"manufacturer",
	"product",
	"serial",
	0,
};

struct crtx_udev_raw2dict_attr_req r2ds[] = {
	{ "usb", "usb_device", usb_attrs },
	{ 0 },
};

char *test_filters[] = {
	"block",
	"partition",
	0,
	0,
};

static int udev_test_handler(struct crtx_event *event, void *userdata, void **sessiondata) {
	struct crtx_dict *dict;
	
	
	dict = crtx_udev_raw2dict((struct udev_device *) crtx_event_get_ptr(event), r2ds, 0);
	
	crtx_print_dict(dict);
	
	// 	char r, *s;
	// 	r = crtx_dict_locate_value(dict, "usb-usb_device/idVendor", 's', &s, sizeof(void*));
	// 	printf("%d %s\n", r, s);
	
	crtx_dict_unref(dict);
	
	return 0;
}

struct crtx_udev_listener ulist;

int main(int argc, char **argv) {
	int ret;
	
	
	crtx_init();
	crtx_handle_std_signals();
	
	memset(&ulist, 0, sizeof(struct crtx_udev_listener));
	
	ulist.sys_dev_filter = test_filters;
	
	ret = crtx_setup_listener("udev", &ulist);
	if (ret) {
		CRTX_ERROR("create_listener(udev) failed: %s\n", strerror(-ret));
		exit(1);
	}
	
	crtx_create_task(ulist.base.graph, 0, "udev_test", udev_test_handler, 0);
	
	ret = crtx_start_listener(&ulist.base);
	if (ret) {
		CRTX_ERROR("starting udev listener failed\n");
		return 1;
	}
	
	crtx_loop();
	
	crtx_finish();
	
	return 0;
}
