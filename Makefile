# Makefile for temperature reader
CC=gcc
CFLAGS=-Wall

all: aprstemper

aprstemper: temper.c pcsensor.c pcsensor.h
	$(CC) $(CFLAGS) temper.c pcsensor.c -o aprstemper -lusb

clean:
	rm aprstemper

install:
	install aprstemper /usr/local/bin

uninstall:
	rm -f /usr/local/bin/aprstemper
