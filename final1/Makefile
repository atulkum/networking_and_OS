CC = gcc
CFLAGS = -g -Wall
LIBS = -lnsl -lsocket -lresolv

all: sv_node 

sv_node: check.o notify.o join.o status.o keepalive.o hello.o sv_node.o common.o list.o 
	$(CC) $(CFLAGS) sv_node.o check.o notify.o join.o status.o hello.o keepalive.o common.o list.o -o sv_node $(LIBS) -L/home/scf-22/csci551b/openssl/lib -lcrypto

sv_node.o: sv_node.c sv_node.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) sv_node.c	 

hello.o: hello.c hello.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) hello.c	 

keepalive.o: keepalive.c keepalive.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) keepalive.c	 

status.o: status.c status.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) status.c	 
join.o: join.c join.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) join.c	 

notify.o: notify.c notify.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) notify.c	 
check.o: check.c check.h
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) check.c	 


common.o: common.c common.h 
	$(CC) -c -I/home/scf-22/csci551b/openssl/include $(CFLAGS) common.c	

list.o: list.c list.h
	$(CC) -c $(CFLAGS) list.c	

clean:
	rm -f sv_node *~ *.o *.bak core
