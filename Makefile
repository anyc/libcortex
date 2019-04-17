#
# Mario Kicherer (dev@kicherer.org) 2016
#
#
 
local_mk ?= Makefile.local
local_mk_rules ?= Makefile.local_rules

SHELL=/bin/bash
PKG_CONFIG?=pkg-config
INSTALL?=install

prefix?=/usr
libdir?=$(prefix)/lib
plugindir?=$(libdir)
includedir?=$(prefix)/include

APP=cortexd
SHAREDLIB=libcrtx.so

OBJS+=core.o \
	dict.o \
	llist.o dllist.o evloop.o

AVAILABLE_TESTS=avahi can curl epoll evdev evloop_qt libvirt nl_libnl netlink_ge \
	nl_route_raw pulseaudio sdbus sip timer udev uevents v4l xcb_randr writequeue

CFLAGS+=$(DEBUG_FLAGS) -D_FILE_OFFSET_BITS=64 -fPIC -DCRTX_PLUGIN_DIR=\"$(plugindir)\"
CXXFLAGS+=$(DEBUG_FLAGS)
LDLIBS+=-lpthread -ldl

#
# make rules
#

.PHONY: clean

all: $(local_mk) core_modules.h plugins shared layer2 crtx_include_dir

#
# module-specific flags
#

include Makefile.modules

# default module sets
BUILTIN_MODULES=signals epoll
STATIC_TOOLS+=cache dict_inout dict_inout_json event_comm socket threads
STATIC_MODULES+=fanotify inotify netlink_raw nl_route_raw uevents socket_raw \
	timer writequeue pipe
DYN_MODULES+=avahi can curl evdev evloop_qt libvirt netlink_ge nl_libnl nf_queue \
	pulseaudio readline sdbus sip udev v4l xcb_randr sdbus_notifications

LAYER2_MODULES?=dynamic_evdev netif

DEFAULT_EVLOOP=epoll

-include $(local_mk)

CFLAGS+=-DDEFAULT_EVLOOP=\"$(DEFAULT_EVLOOP)\"

# build tests for enabled modules
TESTS+=$(foreach t,$(BUILTIN_MODULES) $(STATIC_MODULES) $(DYN_MODULES),$(if $(findstring $(t),$(AVAILABLE_TESTS)), $(t).test))

OBJS+=$(patsubst %,%.o,$(BUILTIN_MODULES)) $(patsubst %,%.o,$(STATIC_MODULES)) $(patsubst %,%.o,$(STATIC_TOOLS))

DYN_MODULES_LIST=$(patsubst %,libcrtx_%.so, $(DYN_MODULES))

# determine missing module dependencies
ACTIVE_MODULES=$(STATIC_TOOLS) $(STATIC_MODULES) $(DYN_MODULES)
MISSING_DEPS=$(strip $(foreach m,$(ACTIVE_MODULES), $(if $(DEPS_$(m)),$(if $(foreach d,$(DEPS_$(m)),$(findstring $(d),$(STATIC_TOOLS) $(STATIC_MODULES) $(DYN_MODULES))),,$(m))) ))
ifneq ($(MISSING_DEPS),)
$(error error missing dep(s) for module(s) $(MISSING_DEPS): $(foreach m,$(MISSING_DEPS),$(foreach d,$(DEPS_$(m)), $(if $(findstring $(d),$(STATIC_TOOLS) $(STATIC_MODULES) $(DYN_MODULES)),,$(d)) )))
endif

# #
# # make rules
# #
# 
# .PHONY: clean
# 
# all: $(local_mk) core_modules.h plugins shared layer2 crtx_include_dir

$(APP): $(OBJS)

shared: $(SHAREDLIB)

$(local_mk):
	cp Makefile.local.skel $(local_mk)

clean:
	$(MAKE) -C layer2 clean
	rm -rf *.o $(APP) core_modules.h $(TESTS) $(SHAREDLIB) libcrtx_*.so

debug:
	$(MAKE) $(MAKEFILE) DEBUG_FLAGS="-g -g3 -gdwarf-2 -DDEBUG -Wall" #-Werror 


crtx_include_dir:
	mkdir -p include/cortex include/cortex/layer2/
	[ -e include/crtx ] || ln -s cortex include/crtx
	for h in *.h; do [ -e include/cortex/$$h ] || ln -s ../../$$h include/cortex/; done
	for h in layer2/*.h; do [ -e include/cortex/$$h ] || ln -s ../../../$$h include/cortex/layer2/; done

core_modules.h: $(local_mk)
	echo -n "" > $@
	for m in $(BUILTIN_MODULES); do echo -e "#include \"$${m}.h\"" >> $@; done
	for m in $(STATIC_TOOLS); do echo -e "#ifdef STATIC_$${m}\n#include \"$${m}.h\"\n#endif" >> $@; done
	for m in $(STATIC_MODULES); do echo -e "#ifdef STATIC_$${m}\n#include \"$${m}.h\"\n#endif" >> $@; done
	
	echo "" >> $@
	echo "struct crtx_module static_modules[] = {" >> $@
	for m in $(BUILTIN_MODULES); do echo -e "\t{\"$${m}\", &crtx_$${m}_init, &crtx_$${m}_finish}," >> $@; done
	for m in $(STATIC_MODULES); do echo -e "#ifdef STATIC_$${m}\n\t{\"$${m}\", &crtx_$${m}_init, &crtx_$${m}_finish},\n#endif" >> $@; done
	echo -e "\t{0, 0, 0}," >> $@
	echo "};" >> $@
	
	echo "" >> $@
	echo "struct crtx_listener_repository static_listener_repository[] = {" >> $@
	for m in $(BUILTIN_MODULES); do echo -e "\t{\"$${m}\", &crtx_new_$${m}_listener}," >> $@; done
	for m in $(STATIC_MODULES); do echo -e "#ifdef STATIC_$${m}\n\t{\"$${m}\", &crtx_new_$${m}_listener},\n#endif" >> $@; done
	echo -e "\t{0, 0}," >> $@
	echo "};" >> $@

dllist.o: CFLAGS+=-DCRTX_DLL
dllist.o: llist.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: CFLAGS+=$(CFLAGS_${@:.o=})
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: CXXFLAGS+=$(CXXFLAGS_${@:.o=})
%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(SHAREDLIB): LDLIBS+=$(foreach o,$(STATIC_TOOLS) $(STATIC_MODULES), $(LDLIBS_$(o)) )
$(SHAREDLIB): $(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

# libcrtx_%.so: $(OBJS_$(patsubst libcrtx_%.so,%,$@))
libcrtx_%.so: LDLIBS+=$(LDLIBS_$(patsubst libcrtx_%.so,%,$@)) $(foreach d,$(DEPS_$(patsubst libcrtx_%.so,%,$@)),$(if $(findstring $(d),$(DYN_MODULES)),$(LDLIBS_$(d))) )
libcrtx_%.so: %.o $(SHAREDLIB)
	$(CC) -shared $(LDFLAGS) $< -o $@ -L. $(LDLIBS) -lcrtx

plugins: $(SHAREDLIB) $(DYN_MODULES_LIST)



tests: $(SHAREDLIB) $(TESTS) layer2_tests

%.test: CFLAGS+=-DCRTX_TEST -g -g3 -gdwarf-2 -DDEBUG -Wall $(CFLAGS_${@:.test=})
%.test: LDLIBS+=$(LDLIBS_${@:.test=}) $(foreach d,${@:.test=} $(DEPS_${@:.test=}),$(if $(findstring $(d),$(DYN_MODULES)),-lcrtx_$(d)))
%.test: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ -L. $(LDFLAGS) $(LDLIBS) -lcrtx

%.test: CXXFLAGS+=-DCRTX_TEST -g -g3 -gdwarf-2 -DDEBUG -Wall $(CXXFLAGS_${@:.test=})
%.test: LDLIBS+=$(LDLIBS_${@:.test=}) $(foreach d,${@:.test=} $(DEPS_${@:.test=}),$(if $(findstring $(d),$(DYN_MODULES)),-lcrtx_$(d)))
%.test: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@ -L. $(LDFLAGS) $(LDLIBS) -lcrtx
	
examples/libcrtx_%.so: examples/control_%.c
	$(CC) -shared -o $@ -fPIC $< $(LDFLAGS) $(CFLAGS) -I. $(LDLIBS)

crtx_examples:
	$(MAKE) -C examples

layer2: $(SHAREDLIB)
	$(MAKE) -C layer2 LAYER2_MODULES="$(LAYER2_MODULES)"

layer2_tests: $(SHAREDLIB)
	$(MAKE) -C layer2 tests LAYER2_MODULES="$(LAYER2_MODULES)"

install:
	$(INSTALL) -m 755 -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 755 -d $(DESTDIR)$(includedir)/cortex/
	$(INSTALL) -m 755 -d $(DESTDIR)$(includedir)/cortex/layer2/
	$(INSTALL) -m 755 -d $(DESTDIR)$(plugindir)
	
	$(INSTALL) -m 755 libcrtx.so $(DESTDIR)$(libdir)
	$(INSTALL) -m 755 libcrtx_*.so $(DESTDIR)$(plugindir)
	
	$(INSTALL) -m 644 *.h $(DESTDIR)$(includedir)/cortex/
	$(INSTALL) -m 644 layer2/*.h $(DESTDIR)$(includedir)/cortex/layer2/

-include $(local_mk_rules)
