
local_mk ?= $(shell pwd)/Makefile.local
export local_mk

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

.PHONY: examples layer2

all: libcortex layer2 examples

$(local_mk):
	cp Makefile.local.skel $(local_mk)

local: $(local_mk)
	echo "prefix=\"$(shell pwd)/local\"" >> $(local_mk)
	echo "exec_prefix=\"$(shell pwd)/local\"" >> $(local_mk)
	echo "LDFLAGS+=\"-Wl,-rpath,\$$(plugindir)\"" >> $(local_mk)

install_local: $(local_mk) all
	$(MAKE) $(MAKEFILE) DESTDIR= install

clean:
	$(MAKE) -C layer2 clean LAYER2_MODULES="$(LAYER2_MODULES)"
	$(MAKE) -C examples clean
	$(MAKE) -C src clean

examples: libcortex $(local_mk)
	$(MAKE) -C examples

layer2: libcortex $(local_mk)
	$(MAKE) -C layer2

layer2_tests: crtx_layer2 $(local_mk)
	$(MAKE) -C layer2 tests plugindir="$(plugindir)"

libcortex: $(local_mk)
	$(MAKE) -C src

install:
	$(MAKE) -C src install
	$(MAKE) -C examples install
	$(MAKE) -C layer2 install

tests: crtx_layer2_tests
	$(MAKE) -C src tests
	$(MAKE) -C tests

debug:
	$(MAKE) $(MAKEFILE) DEBUG=1 $(DEBUGARGS)

run_tests:
	for x in src/epoll.test src/fork.test; do $${x}; done
