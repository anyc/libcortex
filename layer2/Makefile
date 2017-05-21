
libcrtx_dir=../

PKG_CONFIG=pkg-config

CFLAGS=-I$(libcrtx_dir) -fPIC
LDFLAGS=-L$(libcrtx_dir)

LAYER2_LISTENER=dynamic_evdev

LAYER2_LISTENER_LIST=$(patsubst %,libcrtx_%.so, $(LAYER2_LISTENER))

include $(libcrtx_dir)/Makefile.modules

DEPS_dynamic_evdev=evdev udev

all: $(LAYER2_LISTENER_LIST)

libcrtx_%.so: LDLIBS+=$(foreach d,$(DEPS_$(patsubst libcrtx_%.so,%,$@)),$(LDLIBS_$d))
libcrtx_%.so: %.o
	$(CC) -shared $(LDFLAGS) $< -o $@ $(LDLIBS) -L. -lcrtx

%.o: CFLAGS+=$(foreach d,$(DEPS_${@:.o=}),$(CFLAGS_$d))
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
