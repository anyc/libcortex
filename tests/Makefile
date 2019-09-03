
include ../Makefile.common

libcrtx_dir=../

PKG_CONFIG?=pkg-config

CFLAGS+=-I$(libcrtx_dir) -I$(libcrtx_dir)include
LDFLAGS+=-L$(libcrtx_dir) -lcrtx

tests=fork

all: $(foreach t,$(tests),$(t).test)

clean:
	rm -f *.o $(foreach t,$(tests),$(t).test)

%.test: %.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ -L. $(LDFLAGS) $(LDLIBS)

run_%: %.test
	LD_LIBRARY_PATH=$(libcrtx_dir) ./$<
