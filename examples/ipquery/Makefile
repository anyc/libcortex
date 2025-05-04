
libcrtx_path?=../../src

include ../../Makefile.common

CFLAGS+=-I../../include -g -fPIC -Wall

LDFLAGS+=-L$(libcrtx_path)
LDLIBS+=-lcrtx
LIB_LDFLAGS+=-L$(libcrtx_path)
LIB_LDLIBS+=-lcrtx

NAME=ipquery

SONAME=0.1
LIBFILENAME=lib$(NAME).so
LIBSOFILENAME=$(LIBFILENAME).$(SONAME)

LIB_OBJS=ipquery.o

-include $(local_mk)

all: $(NAME) $(LIBFILENAME)

$(NAME): main.o $(LIBFILENAME)
	$(CC) -o $@ $< $(LDFLAGS) $(LDLIBS) -L. -lipquery

$(LIBSOFILENAME): $(LIB_OBJS)
	$(CC) -shared -o $@ $(LIB_OBJS) $(LIB_LDFLAGS) $(LIB_LDLIBS) -Wl,-soname,$@

$(LIBFILENAME): $(LIBSOFILENAME)
	ln -sf $(LIBSOFILENAME) $@

install:
	$(INSTALL) -m 755 ipquery $(DESTDIR)$(bindir)
	$(INSTALL) -m 755 $(LIBSOFILENAME) $(DESTDIR)$(libdir)

clean:
	rm -f *.o $(NAME) $(LIBFILENAME) $(LIBSOFILENAME)

run:
	LD_LIBRARY_PATH=$(libcrtx_path):. ./$(NAME) $(ARGS)

gdb:
	@LD_LIBRARY_PATH=$(libcrtx_path):. gdb -args ./$(NAME) $(ARGS)

valgrind:
	@LD_LIBRARY_PATH=$(libcrtx_path):. valgrind --leak-check=full ./$(NAME) $(ARGS)
