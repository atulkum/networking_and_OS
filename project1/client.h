#ifndef PROJECT1_551_CLIENT_H
#define PROJECT1_551_CLIENT_H

#include "common.h"
#include <openssl/md5.h>
//routine for processing ADDR message at client side
extern int processADDR(int n_socket, char* url);
//routine for processing FILESIZE message at client side
extern int processFILESIZE(int n_socket, char* filename);
//routine for processing GET message at client side
extern int processGET(int n_socket, char* filename, uint32_t off);
//this routine write 1 byte at a time into the socket
extern int write1Byte(int fd, const unsigned char* vptr, int n );
//this routine read 1 byte at a time from the socket
extern int read1Byte(int fd, const unsigned char* vptr, int n );


extern unsigned char printmsg; //0 = don't print message , 1 = print message

#endif //PROJECT1_551_CLIENT_H
