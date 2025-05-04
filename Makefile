
local_mk ?= Makefile.local

ifeq ($(DEBUG),1)
ifneq ($(SANITIZER), 0)
SANITIZER_FLAGS=-fsanitize=address -fsanitize=undefined
endif
DEBUG_FLAGS+=-g -g3 -gdwarf-2 -DDEBUG -Wall -Werror $(SANITIZER_FLAGS)

CFLAGS+=$(DEBUG_FLAGS)
CXXFLAGS+=$(DEBUG_FLAGS)
LDLIBS+=$(DEBUG_FLAGS)

export CFLAGS
export CXXFLAGS
export LDLIBS
endif

all: libcortex crtx_layer2 crtx_examples

$(local_mk):
	cp Makefile.local.skel $(local_mk)

local: $(local_mk)
	echo "plugindir=\"$(shell pwd)/src\"" >> $(local_mk)
	echo "LDFLAGS+=\"-Wl,-rpath,$(shell pwd)/src\"" >> $(local_mk)

clean:
	$(MAKE) -C layer2 clean LAYER2_MODULES="$(LAYER2_MODULES)"
	$(MAKE) -C examples clean
	$(MAKE) -C src clean

crtx_examples: $(local_mk)
	$(MAKE) -C examples local_mk=../$(local_mk)

crtx_layer2: $(SHAREDLIB) $(local_mk)
	$(MAKE) -C layer2 local_mk=../$(local_mk)

crtx_layer2_tests: $(SHAREDLIB) crtx_layer2 $(local_mk)
	$(MAKE) -C layer2 tests plugindir="$(plugindir)" local_mk=../$(local_mk)

libcortex: $(local_mk)
	$(MAKE) -C src local_mk=../$(local_mk)

install:
	$(MAKE) -C src local_mk=../$(local_mk) install
	$(MAKE) -C examples local_mk=../$(local_mk) install
	$(MAKE) -C layer2 local_mk=../$(local_mk) install

tests: crtx_layer2_tests
	$(MAKE) -C src tests
	$(MAKE) -C tests local_mk=../$(local_mk)

debug:
	$(MAKE) $(MAKEFILE) DEBUG=1 $(DEBUGARGS)

run_tests:
	for x in src/epoll.test src/fork.test; do $${x}; done
