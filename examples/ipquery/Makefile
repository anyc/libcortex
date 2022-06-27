
libcrtx_path?=../../

CFLAGS+=-I$(libcrtx_path)/include -g -fPIC -Wall

LDFLAGS+=-L$(libcrtx_path)
LDLIBS+=-lcrtx
LIB_LDFLAGS+=-L$(libcrtx_path)
LIB_LDLIBS+=-lcrtx

NAME=ipquery

SONAME=0.1
LIBFILENAME=lib$(NAME).so
LIBSOFILENAME=$(LIBFILENAME).$(SONAME)

LIB_OBJS=ipquery.o

all: $(NAME) $(LIBFILENAME)

$(NAME): main.o $(LIBFILENAME)
	$(CC) -o $@ $< $(LDFLAGS) $(LDLIBS) -Wl,-rpath-link=$(libcrtx_path) -L. -lipquery

$(LIBSOFILENAME): $(LIB_OBJS)
	$(CC) -shared -o $@ $(LIB_OBJS) $(LIB_LDFLAGS) $(LIB_LDLIBS) -Wl,-soname,$@

$(LIBFILENAME): $(LIBSOFILENAME)
	ln -sf $(LIBSOFILENAME) $@

clean:
	rm -f *.o $(NAME) $(LIBFILENAME) $(LIBSOFILENAME)

run:
	LD_LIBRARY_PATH=$(libcrtx_path):. ./$(NAME) $(ARGS)

gdb:
	@LD_LIBRARY_PATH=$(libcrtx_path):. gdb -args ./$(NAME) $(ARGS)

valgrind:
	@LD_LIBRARY_PATH=$(libcrtx_path):. valgrind --leak-check=full ./$(NAME) $(ARGS)
