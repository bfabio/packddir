CC=gcc
CCFLAGS=-O2

all:
	$(CC) $(CCFLAGS) -Wall -o upackddir upackddir.c
