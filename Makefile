APP=cortexd

CFLAGS+=$(shell pkg-config --cflags libsystemd libnetfilter_queue)

CFLAGS+=-DSTATIC_SD_BUS $(DEBUG_CFLAGS)

LDLIBS+=$(shell pkg-config --libs libsystemd libnetfilter_queue)
LDLIBS+=-lpthread -lreadline -ldl

.PHONY: clean

all: $(APP) socket_client libcrtx_pfw.so

$(APP): cortexd.o core.o sd_bus.o socket.o nf_queue.o readline.o plugins.o

# $(APP)-client:
# 	$(MAKE) $(MAKEFILE) CFLAGS_SOCKET_CLIENT="-DSOCKET_CLIENT" $(APP)-client-int

# $(APP)-client-int: socket.o
# 	$(CC) $(LDFLAGS) socket.o $(LOADLIBES) $(LDLIBS)

# TODO filter flags
socket_client: socket_client.o

libcrtx_pfw.so: pfw.c
	$(CC) -shared -o $@ -fPIC pfw.c $(LDFLAGS)

clean:
	rm -rf *.o $(APP)

debug:
	$(MAKE) $(MAKEFILE) DEBUG_CFLAGS="-g -g3 -gdwarf-2 -DDEBUG -Wall" #-Werror 
