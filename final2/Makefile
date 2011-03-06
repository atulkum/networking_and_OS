CC = gcc
CFLAGS = -g -Wall
LIBS = -lnsl -lsocket -lresolv

all: sv_node 

sv_node: delete.o searchget.o store.o files.o notify.o status.o keepalive.o hello.o common.o list.o sv_node.o  
	$(CC) $(CFLAGS) sv_node.o delete.o searchget.o store.o files.o notify.o status.o keepalive.o hello.o common.o list.o -o sv_node $(LIBS) -L/home/scf-22/csci551b/openssl/lib -lcrypto

sv_node.o: sv_node.c sv_node.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) sv_node.c	 

delete.o: delete.c delete.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) delete.c	 

searchget.o: searchget.c searchget.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) searchget.c	 

store.o: store.c store.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) store.c	 

files.o: files.c files.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) files.c	 

hello.o: hello.c hello.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) hello.c	 

keepalive.o: keepalive.c keepalive.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) keepalive.c	 

status.o: status.c status.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) status.c	 

notify.o: notify.c notify.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) notify.c	 

common.o: common.c common.h 
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) common.c	

list.o: list.c list.h
	$(CC) -c $(CFLAGS) list.c	

clean:
	rm -f sv_node *~ *.o *.bak core
