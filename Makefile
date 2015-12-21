
local_mk ?= Makefile.local
local_mk_rules ?= Makefile.local_rules
-include $(local_mk)

APP=cortexd

OBJS+=cortexd.o core.o socket.o readline.o controls.o fanotify.o inotify.o

CFLAGS+=$(DEBUG_CFLAGS)

LDLIBS+=-lpthread -lreadline -ldl

LDFLAGS=-rdynamic

CONTROLS+=$(patsubst examples/control_%.c,libcrtx_%.so,$(wildcard examples/*.c))

.PHONY: clean

all: $(local_mk) $(APP) socket_client $(CONTROLS)

$(local_mk):
	cp $(local_mk).skel $(local_mk)

$(APP): $(OBJS)

# $(APP)-client:
# 	$(MAKE) $(MAKEFILE) CFLAGS_SOCKET_CLIENT="-DSOCKET_CLIENT" $(APP)-client-int

# $(APP)-client-int: socket.o
# 	$(CC) $(LDFLAGS) socket.o $(LOADLIBES) $(LDLIBS)

# TODO filter flags
socket_client: socket_client.o

clean:
	rm -rf *.o $(APP) $(CONTROLS)

debug:
	$(MAKE) $(MAKEFILE) DEBUG_CFLAGS="-g -g3 -gdwarf-2 -DDEBUG -Wall" #-Werror 

libcrtx_%.so: examples/control_%.c
	$(CC) -shared -o $@ -fPIC $< $(LDFLAGS) $(CFLAGS) -I.

-include $(local_mk_rules)