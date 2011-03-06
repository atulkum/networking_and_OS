CC = gcc
CFLAGS = -g -Wall
LIBS = -lnsl -lsocket -lresolv

all: server client

client: client.o common.o
	$(CC) $(CFLAGS) client.o common.o -o client $(LIBS) -L/home/scf-22/csci551b/openssl/lib -lcrypto
server: server.o common.o
	$(CC) $(CFLAGS) server.o common.o -o server $(LIBS)
server.o: server.c server.h
	$(CC) -c $(CFLAGS) server.c
client.o: client.c client.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) client.c
common.o: common.c common.h
	$(CC) -c $(CFLAGS) common.c	
clean:
	rm -f client server *~ *.o *.bak core
