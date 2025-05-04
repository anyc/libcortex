
libcrtx_path?=../src

CFLAGS+=-I$(libcrtx_path) -g
LDFLAGS+=-L$(libcrtx_path) -Wl,-rpath "$(libcrtx_path)"
LDLIBS+=-lcrtx

-include $(local_mk)

TEST_SRCS=$(shell find $(SOURCEDIR) -name '*.c')
TESTS=$(patsubst %.c,%.test,$(TEST_SRCS))

all: $(TESTS)

%.test: CFLAGS+=-DCRTX_TEST -g -g3 -gdwarf-2 -DDEBUG -Wall $(CFLAGS_${@:.test=})
%.test: LDLIBS+=$(LDLIBS_${@:.test=}) $(foreach d,${@:.test=} $(DEPS_${@:.test=}),$(if $(findstring $(d),$(DYN_MODULES)),-lcrtx_$(d))) -Wl,-rpath $(shell pwd)
%.test: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ -L. $(LDFLAGS) $(LDLIBS) -lcrtx

%.test: CXXFLAGS+=-DCRTX_TEST -g -g3 -gdwarf-2 -DDEBUG -Wall $(CXXFLAGS_${@:.test=})
%.test: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@ -L. $(LDFLAGS) $(LDLIBS) -lcrtx

udev.test: LDLIBS+=-lcrtx_udev

clean:
	rm *.test