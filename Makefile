
local_mk ?= Makefile.local
local_mk_rules ?= Makefile.local_rules
-include $(local_mk)

APP=cortexd

OBJS+=cortexd.o core.o socket.o readline.o controls.o fanotify.o inotify.o \
	event_comm.o cache.o threads.o signals.o dict.o dict_inout.o \
	llist.o dllist.o timer.o

TESTS+=timer.test

CFLAGS+=$(DEBUG_CFLAGS) -D_FILE_OFFSET_BITS=64 -fPIC

LDLIBS+=-lpthread -lreadline -ldl

LDFLAGS=-rdynamic

CONTROLS+=$(patsubst examples/control_%.c,examples/libcrtx_%.so,$(wildcard examples/*.c))

.PHONY: clean

all: $(local_mk) $(APP) $(CONTROLS)

$(local_mk):
	cp $(local_mk).skel $(local_mk)

$(APP): $(OBJS)

shared: libcrtx.so

# $(APP)-client:
# 	$(MAKE) $(MAKEFILE) CFLAGS_SOCKET_CLIENT="-DSOCKET_CLIENT" $(APP)-client-int

# $(APP)-client-int: socket.o
# 	$(CC) $(LDFLAGS) socket.o $(LOADLIBES) $(LDLIBS)

clean:
	rm -rf *.o $(APP) $(CONTROLS) $(TESTS) libcrtx.so

debug:
	$(MAKE) $(MAKEFILE) DEBUG_CFLAGS="-g -g3 -gdwarf-2 -DDEBUG -Wall" #-Werror 

dllist.o: CFLAGS+=-DCRTX_DLL
dllist.o: llist.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

examples/libcrtx_%.so: examples/control_%.c
	$(CC) -shared -o $@ -fPIC $< $(LDFLAGS) $(CFLAGS) -I. $(LDLIBS)

libcrtx.so: $(OBJS)
	$(CC) -shared -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

tests: $(TESTS)

%.test: CFLAGS+=-DCRTX_TEST
%.test: %.c
	rm -f $(<:.c=.o)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS) -L. -lcrtx
	rm -f $(<:.c=.o)
# 	LD_LIBRARY_PATH=. ./$@ > /dev/null

# timer.test: CFLAGS+=-DCRTX_TEST
# timer.test: timer.o

-include $(local_mk_rules)