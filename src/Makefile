#
# Mario Kicherer (dev@kicherer.org) 2016
#
#

local_mk_rules ?= Makefile.local_rules

# we use bash-specific code
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

AVAILABLE_TESTS=avahi can curl epoll evdev evloop_qt fork libnl libvirt mqtt \
	netlink_ge nl_route_raw popen pulseaudio sdbus sd_journal sip timer \
	uevents v4l xcb_randr writequeue uio inotify fanotify

CFLAGS+=-D_FILE_OFFSET_BITS=64 -fPIC -DCRTX_PLUGIN_DIR=\"$(plugindir)\"
CXXFLAGS+=
LDLIBS+=-lpthread -ldl

#
# make rules
#

.PHONY: clean

all: $(local_mk) core_modules.h config.h plugins shared crtx_include_dir

#
# module-specific flags
#

include Makefile.modules
include Makefile.common

# default module sets
BUILTIN_MODULES=signals epoll
STATIC_TOOLS+=cache dict_inout dict_inout_json threads sm
STATIC_MODULES+=fanotify fork inotify netlink_raw nl_route_raw uevents \
	timer writequeue pipe popen uio
DYN_MODULES+=avahi can curl evdev libnl libvirt netlink_ge nf_queue \
	pulseaudio readline sdbus sd_journal udev v4l \
	sdbus_notifications mqtt

DEFAULT_EVLOOP=epoll

-include $(local_mk)

CFLAGS+=-DDEFAULT_EVLOOP=\"$(DEFAULT_EVLOOP)\" -DSO_VERSION=\"$(SO_VERSION)\" -DMAJOR_VERSION=$(MAJOR_VERSION) -DMINOR_VERSION=$(MINOR_VERSION) -DREVISION=$(REVISION)

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

clean:
	rm -f *.o $(APP) core_modules.h config.h $(TESTS) $(SHAREDLIB) libcrtx*.so* libcortex*.deb

debug:
	$(MAKE) $(MAKEFILE) DEBUG=1 $(DEBUGARGS)

crtx_include_dir:
	mkdir -p include/cortex include/cortex/layer2/
	[ -e include/crtx ] || ln -s cortex include/crtx
	for h in *.h; do [ -e include/cortex/$$h ] || ln -s ../../$$h include/cortex/; done
	for h in ../layer2/*.h; do [ -e include/cortex/layer2/$(shell basename \"$$h\") ] || ln -s ../../../$$h include/cortex/layer2/; done

core_modules.h: $(local_mk)
	echo -n "" > $@
	for m in $(BUILTIN_MODULES); do echo -e "#include \"$${m}.h\"" >> $@; done
# 	for m in $(STATIC_TOOLS); do echo -e "#ifdef STATIC_$${m}\n#include \"$${m}.h\"\n#endif" >> $@; done
# 	for m in $(STATIC_MODULES); do echo -e "#ifdef STATIC_$${m}\n#include \"$${m}.h\"\n#endif" >> $@; done
	for m in $(STATIC_TOOLS); do echo -e "#include \"$${m}.h\"" >> $@; done
	for m in $(STATIC_MODULES); do echo -e "#include \"$${m}.h\"" >> $@; done
	
	echo "" >> $@
	echo "struct crtx_module static_modules[] = {" >> $@
	m=threads; echo -e "\t{\"$${m}\", &crtx_$${m}_init, &crtx_$${m}_finish}," >> $@
	for m in $(BUILTIN_MODULES); do echo -e "\t{\"$${m}\", &crtx_$${m}_init, &crtx_$${m}_finish}," >> $@; done
# 	for m in $(STATIC_MODULES); do echo -e "#ifdef STATIC_$${m}\n\t{\"$${m}\", &crtx_$${m}_init, &crtx_$${m}_finish},\n#endif" >> $@; done
# 	for m in $(STATIC_TOOLS); do echo -e "#ifdef STATIC_$${m}\n\t{\"$${m}\", &crtx_$${m}_init, &crtx_$${m}_finish},\n#endif" >> $@; done
	for m in $(STATIC_MODULES); do echo -e "\t{\"$${m}\", &crtx_$${m}_init, &crtx_$${m}_finish}," >> $@; done
	echo -e "\t{0, 0, 0}," >> $@
	echo "};" >> $@
	
	echo "" >> $@
	echo "struct crtx_listener_repository static_listener_repository[] = {" >> $@
	for m in $(BUILTIN_MODULES); do echo -e "\t{\"$${m}\", &crtx_setup_$${m}_listener}," >> $@; done
# 	for m in $(STATIC_MODULES); do echo -e "#ifdef STATIC_$${m}\n\t{\"$${m}\", &crtx_new_$${m}_listener},\n#endif" >> $@; done
	for m in $(STATIC_MODULES); do echo -e "\t{\"$${m}\", &crtx_setup_$${m}_listener}," >> $@; done
	echo -e "\t{0, 0}," >> $@
	echo "};" >> $@

config.h: $(local_mk)
	echo "" > $@
	for m in $(STATIC_MODULES); do echo -e "#define CRTX_HAVE_MOD_$$(echo $${m} | tr a-z A-Z) 1" >> $@; done

dllist.o: CFLAGS+=-DCRTX_DLL
dllist.o: llist.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: CFLAGS+=$(CFLAGS_${@:.o=})
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: CXXFLAGS+=$(CXXFLAGS_${@:.o=})
%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# fix race condition
core.o: core.c core_modules.h config.h

$(SHAREDLIB): LDLIBS+=$(foreach o,$(STATIC_TOOLS) $(STATIC_MODULES), $(LDLIBS_$(o)) )
$(SHAREDLIB): $(OBJS)
	$(CC) -shared -o $@.$(SO_VERSION) $(OBJS) $(LDFLAGS) $(LDLIBS) -Wl,-soname,$@.$(SO_VERSION)
	ln -sf $@.$(SO_VERSION) $@

# libcrtx_%.so: $(OBJS_$(patsubst libcrtx_%.so,%,$@))
libcrtx_%.so: LDLIBS+=$(LDLIBS_$(patsubst libcrtx_%.so,%,$@)) $(foreach d,$(DEPS_$(patsubst libcrtx_%.so,%,$@)),$(if $(findstring $(d),$(DYN_MODULES)),$(LDLIBS_$(d))) )
libcrtx_%.so: %.o $(SHAREDLIB)
	$(CC) -shared $(LDFLAGS) $< -o $@.$(SO_VERSION) -L. $(LDLIBS) -lcrtx -Wl,-soname,$@.$(SO_VERSION)
	ln -sf $@.$(SO_VERSION) $@

plugins: $(SHAREDLIB) $(DYN_MODULES_LIST)



tests: $(TESTS)

%.test: CFLAGS+=-DCRTX_TEST -g -g3 -gdwarf-2 -DDEBUG -Wall $(CFLAGS_${@:.test=})
%.test: LDLIBS+=$(LDLIBS_${@:.test=}) $(foreach d,${@:.test=} $(DEPS_${@:.test=}),$(if $(findstring $(d),$(DYN_MODULES)),-lcrtx_$(d))) -Wl,-rpath $(shell pwd)
%.test: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ -L. $(LDFLAGS) $(LDLIBS) -lcrtx

%.test: CXXFLAGS+=-DCRTX_TEST -g -g3 -gdwarf-2 -DDEBUG -Wall $(CXXFLAGS_${@:.test=})
%.test: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@ -L. $(LDFLAGS) $(LDLIBS) -lcrtx

install: install-lib install-devel

install-devel:
	$(INSTALL) -m 755 -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 755 -d $(DESTDIR)$(plugindir)
	$(INSTALL) -m 755 -d $(DESTDIR)$(includedir)/cortex/
	
	cp -P libcrtx.so $(DESTDIR)$(libdir)
	ls libcrtx_*.so.* 2>/dev/null && cp -P libcrtx_*.so $(DESTDIR)$(plugindir) || :
	
	$(INSTALL) -m 644 *.h $(DESTDIR)$(includedir)/cortex/
	ln -s cortex $(DESTDIR)$(includedir)/crtx
	
	$(INSTALL) -m 644 llist.c $(DESTDIR)$(includedir)/cortex/

install-lib:
	$(INSTALL) -m 755 -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 755 -d $(DESTDIR)$(plugindir)
	
	$(INSTALL) -m 755 libcrtx.so.* $(DESTDIR)$(libdir)
	ls libcrtx_*.so.* 2>/dev/null && $(INSTALL) -m 755 libcrtx_*.so.* $(DESTDIR)$(plugindir) || :

deb: deb-lib deb-devel
	

deb-lib:
	bash -c 'TMP="$(shell mktemp -d)"; \
	echo $${TMP}; \
	$(MAKE) $(MAKEFILE) install-lib DEBUG_FLAGS=$(DEBUG_FLAGS) DESTDIR=$${TMP}; \
	mkdir $${TMP}/DEBIAN; \
	echo -e "Package: libcortex$(shell echo "$(MAJOR_VERSION)" | sed "s/\.//")\\n\
	Version: $(MAJOR_VERSION).$(MINOR_VERSION).$(shell git rev-parse --short HEAD)\\n\
	Section: base\\n\
	Priority: optional\\n\
	Architecture: all\\n\
	Replaces: libcortex\\n\
	Maintainer: unknown@unknown.com\\n\
	Description: libcortex" > $${TMP}/DEBIAN/control; \
	cat $${TMP}/DEBIAN/control; \
	dpkg-deb --build $${TMP} libcortex$(shell echo "$(MAJOR_VERSION)" | sed "s/\.//")-$(MAJOR_VERSION).$(MINOR_VERSION).$(shell git rev-parse --short HEAD)_all.deb; \
	rm -r $${TMP}; '
	
	dpkg-deb -c libcortex$(shell echo "$(MAJOR_VERSION)" | sed "s/\.//")-$(MAJOR_VERSION).$(MINOR_VERSION).$(shell git rev-parse --short HEAD)_all.deb

deb-devel:
	bash -c 'TMP="$(shell mktemp -d)"; \
	echo $${TMP}; \
	$(MAKE) $(MAKEFILE) install-devel DEBUG_FLAGS=$(DEBUG_FLAGS) DESTDIR=$${TMP}; \
	mkdir $${TMP}/DEBIAN; \
	echo -e "Package: libcortex-devel\\n\
	Version: $(MAJOR_VERSION).$(MINOR_VERSION).$(shell git rev-parse --short HEAD)\\n\
	Section: base\\n\
	Priority: optional\\n\
	Architecture: all\\n\
	Maintainer: unknown@unknown.com\\n\
	Description: libcortex-devel" > $${TMP}/DEBIAN/control; \
	cat $${TMP}/DEBIAN/control; \
	dpkg-deb --build $${TMP} libcortex-devel_$(MAJOR_VERSION).$(MINOR_VERSION).$(shell git rev-parse --short HEAD)_all.deb; \
	rm -r $${TMP}; '
	
	dpkg-deb -c libcortex-devel_$(MAJOR_VERSION).$(MINOR_VERSION).$(shell git rev-parse --short HEAD)_all.deb

-include $(local_mk_rules)
