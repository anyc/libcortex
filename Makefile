
local_mk ?= Makefile.local
-include $(local_mk)

APP=cortexd

OBJS+=cortexd.o core.o socket.o readline.o plugins.o

CFLAGS+=$(DEBUG_CFLAGS)

LDLIBS+=-lpthread -lreadline -ldl

LDFLAGS=-rdynamic

PLUGINS=libcrtx_pfw.so

.PHONY: clean

all: $(local_mk) $(APP) socket_client $(PLUGINS)

$(local_mk):
	cp $(local_mk).skel $(local_mk)

$(APP): $(OBJS)

# $(APP)-client:
# 	$(MAKE) $(MAKEFILE) CFLAGS_SOCKET_CLIENT="-DSOCKET_CLIENT" $(APP)-client-int

# $(APP)-client-int: socket.o
# 	$(CC) $(LDFLAGS) socket.o $(LOADLIBES) $(LDLIBS)

# TODO filter flags
socket_client: socket_client.o

libcrtx_pfw.so: pfw.c
	$(CC) -shared -o $@ -fPIC pfw.c $(LDFLAGS) $(CFLAGS)

clean:
	rm -rf *.o $(APP) $(PLUGINS)

debug:
	$(MAKE) $(MAKEFILE) DEBUG_CFLAGS="-g -g3 -gdwarf-2 -DDEBUG -Wall" #-Werror 
