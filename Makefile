
local_mk ?= Makefile.local

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
	$(MAKE) -C layer2 LAYER2_MODULES="$(LAYER2_MODULES)" local_mk=../$(local_mk)

crtx_layer2_tests: $(SHAREDLIB) crtx_layer2 $(local_mk)
	$(MAKE) -C layer2 tests LAYER2_MODULES="$(LAYER2_MODULES)" plugindir="$(plugindir)" local_mk=../$(local_mk)

libcortex: $(local_mk)
	$(MAKE) -C src local_mk=../$(local_mk)

tests: crtx_layer2_tests
	$(MAKE) -C src tests