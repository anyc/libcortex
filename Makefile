
local_mk ?= Makefile.local
local_mk_rules ?= Makefile.local_rules

APP=cortexd

OBJS+=cortexd.o core.o socket.o socket_raw.o controls.o fanotify.o inotify.o \
	event_comm.o cache.o threads.o signals.o dict.o dict_inout.o \
	llist.o dllist.o timer.o epoll.o

TESTS+=timer.test epoll.test

CFLAGS+=$(DEBUG_CFLAGS) -D_FILE_OFFSET_BITS=64 -fPIC

LDLIBS+=-lpthread -ldl

# LDFLAGS=-rdynamic

# CONTROLS+=$(patsubst examples/control_%.c,examples/libcrtx_%.so,$(wildcard examples/*.c))




CFLAGS_udev=$(shell pkg-config --cflags libudev)
LDLIBS_udev=$(shell pkg-config --libs libudev)

CFLAGS_libvirt=$(shell pkg-config --cflags libvirt)
LDLIBS_libvirt=$(shell pkg-config --libs libvirt)

-include $(local_mk)

CFLAGS+=$(patsubst %,-DSTATIC_%,$(STATIC_MODULES))

OBJS+=$(patsubst %,%.o,$(STATIC_MODULES))

PLUGIN_LIST=$(patsubst %,libcrtx_%.so, $(PLUGINS))

.PHONY: clean

all: $(local_mk) $(APP) $(CONTROLS) $(PLUGIN_LIST)

$(local_mk):
	cp $(local_mk).skel $(local_mk)

$(APP): $(OBJS)

shared: libcrtx.so

# $(APP)-client:
# 	$(MAKE) $(MAKEFILE) CFLAGS_SOCKET_CLIENT="-DSOCKET_CLIENT" $(APP)-client-int

# $(APP)-client-int: socket.o
# 	$(CC) $(LDFLAGS) socket.o $(LOADLIBES) $(LDLIBS)

clean:
	rm -rf *.o $(APP) $(CONTROLS) $(TESTS) libcrtx.so libcrtx_*.so

debug:
	$(MAKE) $(MAKEFILE) DEBUG_CFLAGS="-g -g3 -gdwarf-2 -DDEBUG -Wall" #-Werror 

dllist.o: CFLAGS+=-DCRTX_DLL
dllist.o: llist.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

examples/libcrtx_%.so: examples/control_%.c
	$(CC) -shared -o $@ -fPIC $< $(LDFLAGS) $(CFLAGS) -I. $(LDLIBS)

libcrtx.so: $(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

tests: libcrtx.so $(TESTS)

%.test: CFLAGS+=-DCRTX_TEST -g -g3 -gdwarf-2 -DDEBUG -Wall
%.test: %.c
	rm -f $(<:.c=.o)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS) -L. -lcrtx
	rm -f $(<:.c=.o)
# 	LD_LIBRARY_PATH=. ./$@ > /dev/null


crtx_examples:
	$(MAKE) -C examples

# libcrtx_%.so: %.o
# 	$(CC) -shared $(LDFLAGS) $< -o $< $(LDLIBS)
# # libcrtx_$(<:%.o=%).so
# libcrtx_udev.so: CFLAGS+=$(shell pkg-config --cflags libudev)
# libcrtx_udev.so: LDLIBS+=$(shell pkg-config --libs libudev)
# libcrtx_udev.so: udev.c

libcrtx_%.so: CFLAGS+=$(CFLAGS_%)
libcrtx_%.so: LDLIBS+=$(LDLIBS_%)
libcrtx_%.so: %.o
	$(CC) -shared $(LDFLAGS) $< -o $@ $(LDLIBS)

plugins: $(PLUGIN_LIST)

-include $(local_mk_rules)
