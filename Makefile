bstrlib_dir = $(HOME)/swtools/text/bstrlib

CC = gcc
CFLAGS += -Wall -g

includes = -I$(bstrlib_dir)/include -I.
libs = -L$(bstrlib_dir)/lib -lbstrlib
objs = util.o 

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
