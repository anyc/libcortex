
libcrtx_path?=../

CFLAGS+=-I$(libcrtx_path) -g
LDFLAGS+=-L$(libcrtx_path)

LDLIBS+=-lcrtx

EXAMPLES=libvirt-usb-hotplug control_inotify_notify control_fanotify control_notifyd control_pfw

all: $(EXAMPLES)

libvirt-usb-hotplug: CFLAGS+=$(shell pkg-config --cflags libvirt) $(shell xml2-config --cflags)
libvirt-usb-hotplug: LDLIBS+=$(shell pkg-config --libs libvirt) $(shell xml2-config --libs) -lcrtx_udev
libvirt-usb-hotplug: libvirt-usb-hotplug.c

clean:
	rm -f *.o $(EXAMPLES)

run_%: %
	LD_LIBRARY_PATH=$(libcrtx_path) ./$<

gdb_%: %
	LD_LIBRARY_PATH=$(libcrtx_path) gdb -args ./$<

valgrind_%: %
	LD_LIBRARY_PATH=$(libcrtx_path) valgrind --leak-check=full ./$<
