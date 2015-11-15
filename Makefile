APP=cortexd

CFLAGS+=$(shell pkg-config --cflags libsystemd)

CFLAGS+=-DSTATIC_SD_BUS

CFLAGS+=-g

LDLIBS+=$(shell pkg-config --libs libsystemd)
LDLIBS+=-lpthread

.PHONY: clean

all: $(APP) socket_client

$(APP): cortexd.o core.o sd_bus.o socket.o

# $(APP)-client:
# 	$(MAKE) $(MAKEFILE) CFLAGS_SOCKET_CLIENT="-DSOCKET_CLIENT" $(APP)-client-int

# $(APP)-client-int: socket.o
# 	$(CC) $(LDFLAGS) socket.o $(LOADLIBES) $(LDLIBS)

# TODO filter flags
socket_client: socket_client.o

clean:
	rm -rf *.o $(APP)