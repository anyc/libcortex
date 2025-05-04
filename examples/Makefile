
libcrtx_path?=../src

prefix?=/usr
exec_prefix?=/usr
bindir?=$(exec_prefix)/bin

SHELL?=/bin/bash
PKG_CONFIG?=pkg-config
INSTALL?=install

CFLAGS+=-I$(libcrtx_path) -g
LDFLAGS+=-L$(libcrtx_path) -Wl,-rpath "$(libcrtx_path)"

LDLIBS+=-lcrtx

EXAMPLES?=libvirt-usb-hotplug named_pipe dbus-tool
L_EXAMPLES?=$(filter-out ipquery,$(EXAMPLES))

-include $(local_mk)

all: $(EXAMPLES)

libvirt-usb-hotplug: CFLAGS+=$(shell pkg-config --cflags libvirt) $(shell xml2-config --cflags)
libvirt-usb-hotplug: LDLIBS+=$(shell pkg-config --libs libvirt) $(shell xml2-config --libs) -lcrtx_udev
libvirt-usb-hotplug: libvirt-usb-hotplug.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS) $(LDLIBS)
	
libcrtx_ctrl_%.so: control_%.c
	$(CC) -shared -o $@ -fPIC $< $(LDFLAGS) $(CFLAGS) -I. $(LDLIBS)

ipquery:
	make -C ipquery

pfw: CFLAGS+=-DPFW_STANDALONE
pfw: LDLIBS+=-lcrtx_nf_queue
pfw: control_pfw.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS) $(LDLIBS)

dbus-tool: LDLIBS+=-lcrtx_sdbus -lsystemd

clean:
	rm -f *.o $(L_EXAMPLES) $(EXAMPLES)
	make -C ipquery clean

run_%: %
	LD_LIBRARY_PATH=$(libcrtx_path) ./$<

gdb_%: %
	LD_LIBRARY_PATH=$(libcrtx_path) gdb -args ./$<

valgrind_%: %
	LD_LIBRARY_PATH=$(libcrtx_path) valgrind --leak-check=full ./$<

install:
	$(INSTALL) -m 755 -d $(DESTDIR)$(bindir)
	
	$(INSTALL) -m 755 $(EXAMPLES) $(DESTDIR)$(bindir)
