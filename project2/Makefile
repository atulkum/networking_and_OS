CC = gcc
CFLAGS = -g -Wall
LIBS = -lpthread -lm -lnsl

all: mm2

mm2: mm2.o common.o list.o
	$(CC) $(CFLAGS) mm2.o common.o list.o -o mm2 $(LIBS)
mm2.o: mm2.c mm2.h
	$(CC) -c $(CFLAGS) mm2.c
common.o: common.c common.h
	$(CC) -c $(CFLAGS) common.c	
list.o: list.c list.h
	$(CC) -c $(CFLAGS) list.c	
	
clean:
	rm -f client mm2 *~ *.o *.bak core
