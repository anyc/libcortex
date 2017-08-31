
libcrtx_dir=../

PKG_CONFIG=pkg-config

CFLAGS+=-I$(libcrtx_dir) -fPIC
LDFLAGS+=-L$(libcrtx_dir)

LAYER2_LISTENER=dynamic_evdev

LAYER2_LISTENER_LIST=$(patsubst %,libcrtx_%.so, $(LAYER2_LISTENER))

TESTS+=$(foreach t,$(LAYER2_LISTENER), $(t).test)

include $(libcrtx_dir)/Makefile.modules

CFLAGS_dynamic_evdev=$(shell pkg-config --cflags ncurses librrd)
LDLIBS_dynamic_evdev=$(shell pkg-config --libs ncurses librrd)
DEPS_dynamic_evdev=evdev udev

.PHONY: clean

all: $(LAYER2_LISTENER_LIST)

clean:
	rm -rf *.o $(TESTS) libcrtx_*.so

libcrtx_%.so: LDLIBS+=$(foreach d,$(DEPS_$(patsubst libcrtx_%.so,%,$@)),$(LDLIBS_$d))
libcrtx_%.so: %.o
	$(CC) -shared $(LDFLAGS) $< -o $@ $(LDLIBS) -L. -lcrtx
	[ ! -f ../$@ ] && ln -s layer2/$@ ../$@ || true

%.o: CFLAGS+=$(foreach d,$(DEPS_${@:.o=}),$(CFLAGS_$d))
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

tests: $(TESTS)

%.test: CFLAGS+=-DCRTX_TEST -g -g3 -gdwarf-2 -DDEBUG -Wall $(CFLAGS_${@:.test=}) $(foreach d,$(DEPS_${@:.test=}),$(CFLAGS_$d))
%.test: LDLIBS+=$(LDLIBS_${@:.test=}) $(foreach d,${@:.test=} $(DEPS_${@:.test=}),-lcrtx_$(d) $(LDLIBS_$(d)) )
%.test: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS) -L. -lcrtx
