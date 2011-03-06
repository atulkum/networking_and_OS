#ifndef PROJECT1_551_COMMON_H
#define PROJECT1_551_COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>

//The message data structure
typedef struct tagReqMsg {
	uint16_t MsgType;
	uint32_t Offset; //could be non-zero only for GET message request
	uint32_t DataLen;
	char *Data; //in case of ADDR it's url, in case of FILESIZE and GET it's the file name
} ReqMsg;

//getting the dotted ip address from the host name
extern const char* getIPAddr(const char* host);
//routine for printing the hex data
extern void printhex(unsigned char *src ,int n);
//routine for getting the file name
extern int getFileSize(const unsigned char *filePath);
//routine for printing the packet information at the client side
extern void printInfo(ReqMsg *req, int s);
//routine for printing the packet information at the server side
extern void printInfoServer(ReqMsg *req, int s);

#endif //PROJECT1_551_COMMON_H
