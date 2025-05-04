
libcrtx_dir?=../src

include ../Makefile.common

CFLAGS+=-I$(libcrtx_dir) -I../include -g
LDFLAGS+=-L$(libcrtx_dir)

LDLIBS+=-lcrtx

EXAMPLES?=libvirt-usb-hotplug named_pipe dbus-tool ipquery
L_EXAMPLES=$(filter-out ipquery,$(EXAMPLES))

-include $(local_mk)

.PHONY: ipquery

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
	rm -f *.o $(L_EXAMPLES)
	make -C ipquery clean

run_%: %
	LD_LIBRARY_PATH=$(libcrtx_dir) ./$<

gdb_%: %
	LD_LIBRARY_PATH=$(libcrtx_dir) gdb -args ./$<

valgrind_%: %
	LD_LIBRARY_PATH=$(libcrtx_dir) valgrind --leak-check=full ./$<

install:
	[ "$(EXAMPLES)" == "" ] || $(INSTALL) -m 755 -d $(DESTDIR)$(bindir)
	
	if [ "$(L_EXAMPLES)" != "" ]; then $(INSTALL) -m 755 $(L_EXAMPLES) $(DESTDIR)$(bindir); fi
	
	[[ "$(EXAMPLES)" != *ipquery* ]] || $(MAKE) -C ipquery install
