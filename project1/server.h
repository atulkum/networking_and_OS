#ifndef PROJECT1_551_SERVER_H
#define PROJECT1_551_SERVER_H

#include "common.h"
#include <fcntl.h>
#include <sys/wait.h>


extern int processMsg(int n_socket);
//routine for processing ADDR message at server side
extern int processADDR(int n_socket, ReqMsg *req);
//routine for processing FILESIZE message at server side
extern int processFILESIZE(int n_socket, ReqMsg* req);
//routine for processing GET message at server side
extern int processGET(int n_socket, ReqMsg* req);
//routine for sending ALLFAIL message at server side
extern int processALLFAIL(int n_socket);
//this routine write 1 byte at a time into the socket
extern int write1Byte(int fd, const unsigned char* vptr, int n );
//this routine read 1 byte at a time from the socket
extern int read1Byte(int fd, const unsigned char* vptr, int n );


extern unsigned char printmsg; //0 = don't print message , 1 = print message
extern unsigned char isShutdown; //0 = don't shutdown , 1 = shutdown

#endif //PROJECT1_551_SERVER_H
