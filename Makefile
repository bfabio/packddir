CC=gcc
CCFLAGS=-O2

all:
	$(CC) $(CCFLAGS) -Wall -o upackddir upackddir.c
clean:
	rm -f upackddir

install:
	mv upackddir /usr/bin/upackddir
