
local_mk ?= Makefile.local
local_mk_rules ?= Makefile.local_rules

PKG_CONFIG=pkg-config

APP=cortexd
SHAREDLIB=libcrtx.so

OBJS+=cortexd.o core.o socket.o socket_raw.o controls.o fanotify.o inotify.o \
	event_comm.o cache.o threads.o signals.o dict.o dict_inout.o \
	llist.o dllist.o timer.o epoll.o

AVAILABLE_TESTS=avahi can epoll evdev libvirt netlink_ge nl_route pulseaudio sd_bus sip timer udev uevents xcb_randr

# LDFLAGS=-rdynamic
# CONTROLS+=$(patsubst examples/control_%.c,examples/libcrtx_%.so,$(wildcard examples/*.c))

CFLAGS+=$(DEBUG_CFLAGS) -D_FILE_OFFSET_BITS=64 -fPIC
LDLIBS+=-lpthread -ldl

#
# module-specific flags
#

include Makefile.modules

# default module sets
STATIC_MODULES+=netlink_raw nl_route uevents dict_inout_json
DYN_MODULES+=avahi can evdev libvirt netlink_ge nf_queue pulseaudio readline sd_bus sd_bus_notifications sip udev xcb_randr

# LAYER2=crtx_layer2

-include $(local_mk)

# build tests for enabled modules
TESTS+=$(foreach t,$(STATIC_MODULES) $(DYN_MODULES),$(if $(findstring $(t),$(AVAILABLE_TESTS)), $(t).test))

OBJS+=$(patsubst %,%.o,$(STATIC_MODULES))

DYN_MODULES_LIST=$(patsubst %,libcrtx_%.so, $(DYN_MODULES))

# determine missing module dependencies
ACTIVE_MODULES=$(STATIC_MODULES) $(DYN_MODULES)
MISSING_DEPS=$(strip $(foreach m,$(ACTIVE_MODULES), $(if $(DEPS_$(m)),$(if $(foreach d,$(DEPS_$(m)),$(findstring $(d),$(STATIC_MODULES) $(DYN_MODULES))),,$(m))) ))
ifneq ($(MISSING_DEPS),)
$(error error missing dep(s) for module(s) $(MISSING_DEPS): $(foreach m,$(MISSING_DEPS),$(foreach d,$(DEPS_$(m)), $(if $(findstring $(d),$(STATIC_MODULES) $(DYN_MODULES)),,$(d)) )))
endif

#
# make rules
#

.PHONY: clean

all: $(local_mk) $(CONTROLS) plugins shared $(LAYER2)

$(APP): $(OBJS)

shared: $(SHAREDLIB)

$(local_mk):
	cp $(local_mk).skel $(local_mk)

clean:
	$(MAKE) -C layer2 clean
	rm -rf *.o $(APP) $(CONTROLS) $(TESTS) $(SHAREDLIB) libcrtx_*.so

debug:
	$(MAKE) $(MAKEFILE) DEBUG_CFLAGS="-g -g3 -gdwarf-2 -DDEBUG -Wall" #-Werror 



dllist.o: CFLAGS+=-DCRTX_DLL
dllist.o: llist.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: CFLAGS+=$(CFLAGS_${@:.o=})
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(SHAREDLIB): LDLIBS+=$(foreach o,$(STATIC_MODULES), $(LDLIBS_$(o)) )
$(SHAREDLIB): $(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

libcrtx_%.so: LDLIBS+=$(LDLIBS_$(patsubst libcrtx_%.so,%,$@)) $(foreach d,$(DEPS_$(patsubst libcrtx_%.so,%,$@)),$(if $(findstring $(d),$(DYN_MODULES)),$(LDLIBS_$(d))) )
libcrtx_%.so: %.o $(SHAREDLIB)
	$(CC) -shared $(LDFLAGS) $< -o $@ $(LDLIBS) -L. -lcrtx

plugins: $(SHAREDLIB) $(DYN_MODULES_LIST)



tests: $(SHAREDLIB) $(TESTS) crtx_layer2_tests

%.test: CFLAGS+=-DCRTX_TEST -g -g3 -gdwarf-2 -DDEBUG -Wall $(CFLAGS_${@:.test=})
%.test: LDLIBS+=$(LDLIBS_${@:.test=}) $(foreach d,${@:.test=} $(DEPS_${@:.test=}),$(if $(findstring $(d),$(DYN_MODULES)),-lcrtx_$(d)))
%.test: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS) -L. -lcrtx

examples/libcrtx_%.so: examples/control_%.c
	$(CC) -shared -o $@ -fPIC $< $(LDFLAGS) $(CFLAGS) -I. $(LDLIBS)

crtx_examples:
	$(MAKE) -C examples

crtx_layer2: $(SHAREDLIB)
	$(MAKE) -C layer2

crtx_layer2_tests: $(SHAREDLIB)
	$(MAKE) -C layer2 tests
	
-include $(local_mk_rules)
