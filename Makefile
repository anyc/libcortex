
local_mk ?= Makefile.local
local_mk_rules ?= Makefile.local_rules
-include $(local_mk)

APP=cortexd

OBJS+=cortexd.o core.o socket.o readline.o controls.o fanotify.o inotify.o event_comm.o

CFLAGS+=$(DEBUG_CFLAGS) -D_FILE_OFFSET_BITS=64

LDLIBS+=-lpthread -lreadline -ldl

LDFLAGS=-rdynamic

CONTROLS+=$(patsubst examples/control_%.c,examples/libcrtx_%.so,$(wildcard examples/*.c))

.PHONY: clean

all: $(local_mk) $(APP) $(CONTROLS)

$(local_mk):
	cp $(local_mk).skel $(local_mk)

$(APP): $(OBJS)

# $(APP)-client:
# 	$(MAKE) $(MAKEFILE) CFLAGS_SOCKET_CLIENT="-DSOCKET_CLIENT" $(APP)-client-int

# $(APP)-client-int: socket.o
# 	$(CC) $(LDFLAGS) socket.o $(LOADLIBES) $(LDLIBS)

clean:
	rm -rf *.o $(APP) $(CONTROLS)

debug:
	$(MAKE) $(MAKEFILE) DEBUG_CFLAGS="-g -g3 -gdwarf-2 -DDEBUG -Wall" #-Werror 

examples/libcrtx_%.so: examples/control_%.c
	$(CC) -shared -o $@ -fPIC $< $(LDFLAGS) $(CFLAGS) -I.

-include $(local_mk_rules)