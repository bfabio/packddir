CC=gcc
CCFLAGS=-g -Wall
OBJS=lists.o utils.o upackddir.o
DESTDIR=/

all: $(OBJS)
	$(CC) $(CCFLAGS) -o upackddir $(OBJS)

upackddir.o: upackddir.c
	$(CC) $(CCFLAGS) --std=c99 -c upackddir.c

.c.o:
	$(CC) $(CCFLAGS) -c -o $@ $<

clean:
	rm -f upackddir $(OBJS) tags

install:
	install -m 755 upackddir $(DESTDIR)/usr/bin/upackddir
