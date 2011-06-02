bstrlib_dir = $(HOME)/swtools/text/bstrlib

CC = gcc
CFLAGS += -Wall -g

includes = -I$(bstrlib_dir)/include -I.
libs = -L$(bstrlib_dir)/lib -lbstrlib
objs = main.o init.o util.o gdb.o cleanup.o

all: gstep

gstep: $(objs)
	$(CC) $(CFLAGS) -o $@ $(objs) $(libs)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $(includes) -c $<

clean:
	-rm gstep
	-rm *.o
