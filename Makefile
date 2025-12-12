
local_mk ?= $(shell pwd)/Makefile.local
export local_mk

ifeq ($(DEBUG),1)
ifneq ($(SANITIZER),0)
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

include Makefile.common
include src/Makefile.modules

define check_mods
	$(foreach o,$(1), \
		test -z "$(PKGCFG_$(o))" && echo -n "$(o) " || { \
			echo -n "checking $(o)" >&2; \
			$(PKG_CONFIG) --cflags "$(PKGCFG_$(o))" >/dev/null 2>&1 && \
				{ echo -n "$(o) "; echo " YES">&2; } \
				|| echo "">&2; \
			}; \
		)
endef

POSSIBLE_DYN=$(call check_mods,$(DYN_MODULES))
POSSIBLE_STATIC=$(call check_mods,$(STATIC_MODULES))

$(local_mk):
	POSSIBLE_DYN=`$(POSSIBLE_DYN)`; \
	POSSIBLE_STATIC=`$(POSSIBLE_STATIC)`; \
	echo -e "DYN_MODULES=$$POSSIBLE_DYN \n\
STATIC_MODULES=$$POSSIBLE_STATIC \n\
" > $@

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
	$(MAKE) -C tests clean

examples: libcortex $(local_mk)
	$(MAKE) -C examples

layer2: libcortex $(local_mk)
	$(MAKE) -C layer2

layer2_tests: layer2 $(local_mk)
	$(MAKE) -C layer2 tests plugindir="$(plugindir)"

libcortex: $(local_mk)
	$(MAKE) -C src

install:
	$(MAKE) -C src install
	$(MAKE) -C examples install
	$(MAKE) -C layer2 install

tests: layer2_tests
	$(MAKE) -C src tests
	$(MAKE) -C tests

debug:
	$(MAKE) $(MAKEFILE) DEBUG=1 $(DEBUGARGS)

run_tests:
	for x in src/epoll.test tests/fork.test; do $${x}; done
