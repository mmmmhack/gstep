bstrlib_dir = $(HOME)/swtools/text/bstrlib

CC = gcc
CFLAGS += -Wall -g

includes = -I$(bstrlib_dir)/include -I.
libs = -L$(bstrlib_dir)/lib -lbstrlib
objs = util.o gdb.o servsock.o

tests = $(basename $(wildcard test_*.c))

all: gserver gclient

gserver: gserver.o $(objs)
	$(CC) $(CFLAGS) -o $@ gserver.o $(objs) $(libs)

gclient: gclient.o $(objs)
	$(CC) $(CFLAGS) -o $@ gclient.o $(objs) $(libs)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $(includes) -c $<

#.PRECIOUS: %.o

clean:
	-rm gserver gclient
	-rm *.o

.PHONY: test

test: $(tests)
	@for f in $(tests); \
		do [ -x $$f ] && printf "%s: " $$f && ./$$f && printf " OK\n" || printf " FAIL\n"; \
	done

test_time_sub: test_time_sub.o util.o
	$(CC) -o $@ $^

