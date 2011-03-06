#include "common.h"
//This method first checks is the ip address is already in the
//dotted format then it return the passed string otherwise
//it get the ip by calling gethostbyname
const char* getIPAddr(const char* host){
	if(host[0] - '0' > 0 && host[0] - '0' < 9){
		//hostname is in dotted decimal format
		return host;
	}
	else{
		struct hostent *he;
		he = gethostbyname(host);
		if (he == NULL) { // do some error checking
		    //gethostbyname
		    return NULL;
		}
		return inet_ntoa(*(struct in_addr*)he->h_addr);
	}
}
//debugging purpose routine
void printhex(unsigned char *src ,int n){
	/*char *toprint = (char*)malloc(n*2 + 1);
	toprint[n*2] = '\0';
	int i = 0;
	for (; i<n; i++){
		sprintf(toprint+i*2, "%02X", src[i]);
	}
	printf("length %d\n", n);
	printf("message %s\n", toprint);
	free(toprint);*/
}
//routine to get the file size by calling stat
int getFileSize(const unsigned char *filePath){
	struct stat file_stats;
	if(stat((const char*)filePath,&file_stats) == -1){
		//printf("File size not found\n");
	    return -1;
	}
	else{
		//printf("File size is %9jd\n", (intmax_t) file_stats.st_size);
		return file_stats.st_size;
	}
}
//routine for printing header information for client
void printInfo(ReqMsg *req, int s){
	struct sockaddr_in addr;
	socklen_t len = sizeof addr;
	getpeername(s, (struct sockaddr*)&addr, &len);

	printf("\tReceived %d bytes from %s.\n", 10 + req->DataLen, inet_ntoa(addr.sin_addr));
	printf("\t  MessageType: 0x%04x\n", req->MsgType);
	printf("\t       Offset: 0x%08x\n", req->Offset);
	printf("\t   DataLength: 0x%08x\n", req->DataLen);
}
//routine for printing header information for server
void printInfoServer(ReqMsg *req, int s){
	struct sockaddr_in addr;
	socklen_t len = sizeof addr;
	getpeername(s, (struct sockaddr*)&addr, &len);

	printf("Received %d bytes from %s.\n", 10 + req->DataLen, inet_ntoa(addr.sin_addr));
	printf("  MessageType: 0x%04x\n", req->MsgType);
	printf("       Offset: 0x%08x\n", req->Offset);
	printf("   DataLength: 0x%08x\n", req->DataLen);
}
