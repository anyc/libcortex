
libcrtx_dir=../

PKG_CONFIG?=pkg-config

CFLAGS+=$(DEBUG_FLAGS) -I$(libcrtx_dir) -I$(libcrtx_dir)include -fPIC
LDFLAGS+=-L$(libcrtx_dir)

LAYER2_LISTENER_LIST=$(patsubst %,$(libcrtx_dir)/libcrtx_%.so, $(LAYER2_MODULES))

TESTS+=$(foreach t,$(LAYER2_MODULES), $(t).test)

include $(libcrtx_dir)/Makefile.modules
include $(libcrtx_dir)/Makefile.common

# CFLAGS_dynamic_evdev=$(shell pkg-config --cflags ncurses librrd)
# LDLIBS_dynamic_evdev=$(shell pkg-config --libs ncurses librrd)
DEPS_dynamic_evdev=evdev udev

DEPS_forked_curl=curl

CFLAGS_netif=
LDLIBS_netif=
DEPS_netif=libnl

DEPS_netconf=libnl

.PHONY: clean

all: $(LAYER2_LISTENER_LIST)

clean:
	rm -rf *.o $(TESTS) libcrtx_*.so

$(libcrtx_dir)/libcrtx_%.so: LDLIBS+=$(foreach d,$(DEPS_$(patsubst libcrtx_%.so,%,$@)),$(LDLIBS_$d))
$(libcrtx_dir)/libcrtx_%.so: %.o
	$(CC) -shared $(LDFLAGS) $< -o $@.$(SO_VERSION) $(LDLIBS) -L. -lcrtx -Wl,-soname,$(shell basename $@.$(SO_VERSION))
	ln -sf $(shell basename $@.$(SO_VERSION)) $(libcrtx_dir)/$(shell basename $@)
	
	#[ ! -f ../$@ ] && ln -s layer2/$@ ../$@ || true
# 	mv layer2/$@ ../$@

%.o: CFLAGS+=$(foreach d,$(DEPS_${@:.o=}),$(CFLAGS_$d))
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

tests: $(TESTS)

%.test: CFLAGS+=-DCRTX_TEST -g -g3 -gdwarf-2 -DDEBUG -Wall $(CFLAGS_${@:.test=}) $(foreach d,$(DEPS_${@:.test=}),$(CFLAGS_$d))
%.test: LDLIBS+=$(LDLIBS_${@:.test=}) $(foreach d,${@:.test=} $(DEPS_${@:.test=}),-lcrtx_$(d) $(LDLIBS_$(d)) )
%.test: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS) -L. -lcrtx
