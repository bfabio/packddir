CC=gcc
CCFLAGS=-g -Wall
OBJS=lists.o utils.o upackddir.o
DESTDIR=/

all: $(OBJS)
	$(CC) $(CCFLAGS) -o upackddir $(OBJS)

upackddir.o:
	$(CC) $(CCFLAGS) --std=c99 -c upackddir.c

.c.o:
	$(CC) $(CCFLAGS) -c -o $*.o $<

clean:
	rm -f upackddir $(OBJS)

install:
	install -m 755 upackddir $(DESTDIR)/usr/bin/upackddir
