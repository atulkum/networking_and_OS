#include "client.h"
unsigned char printmsg = 0;
//host name of the server
char hostName[512 + 1];

//the main method parse the command line and connect to the server
int main(int argc, char *argv[]){
	char msgType = -1; //1 = ADR_REQ, 2 = FSZ_REQ, 3 = GET_REQ
	uint16_t serverPort = 0;
	uint32_t offset;
	char* data = NULL;
	int isOffset = 0;

	if(argc < 4){
		fprintf(stderr, "Error: malformed command.\n");
		exit(0);
	}
	//parse the command line one by one
	//escaping the executable name
	argc--; argv++;
	//get the message type
	if (!strcmp(*argv, "adr")) {
		msgType = 1;
	}
	else if (!strcmp(*argv, "fsz")) {
		msgType = 2;
	}
	else if (!strcmp(*argv, "get")) {
		msgType = 3;
	}
	else{
		fprintf(stderr, "Error: malformed command.\n");
		exit(0);
	}

	argc--; argv++;
	//the number of arguments consumed in one iteration
	int arguments = 1;
	for(;argc > 0; argc-=arguments, argv += arguments){
		 //parse for port number and server host name
		 if ((serverPort == 0) && strchr(*argv, ':') != NULL) {
			char *temp = strchr(*argv, ':');
			*temp = '\0';

			if(strlen(*argv) == strlen(temp)){
				fprintf(stderr, "Error: malformed command.\n");
				exit(0);
			}
			if(isalpha(*(temp + 1))){
				fprintf(stderr, "Error: bad pot number.\n");
				exit(0);
			}
			serverPort = atoi(temp + 1);
			strcpy(hostName, *argv);
			arguments = 1;
		}
		else if (!strcmp(*argv, "-m")) {
			printmsg = 1;
			arguments = 1;
		}
		else if(!strcmp(*argv, "-o")){
			if(argc == 1){
				fprintf(stderr, "Error: malformed command.\n");
				exit(0);
			}
			if(isalpha(*(*(argv + 1)))){
				fprintf(stderr, "Error: bad offset argument.\n");
				exit(0);
			}
			offset = atoi(*(argv + 1));
			arguments = 2;
			isOffset = 1;
		}
		else if(data == NULL){
			if(strlen(*argv) > 512){
				fprintf(stderr, "Error: hostname too big.\n");
				exit(0);
			}
			data = *argv;
		}
		else{
			fprintf(stderr, "Error: malformed command.\n");
			exit(0);
		}
	}
	//if message type is not set or data is null or server port is not set
	//return error and exit
	if( msgType == -1 || data == NULL || serverPort == 0 ){
		fprintf(stderr, "Error: malformed command.\n");
		exit(0);
	}
	//if offset is set for ADDR and FILESIZE return error and exit
	if((isOffset == 1) &&  ((msgType == 1) ||  (msgType == 2))){
		fprintf(stderr, "Error: -o option with adr and fsz is an error.\n");
		exit(0);
	}
	//connect to the server
	struct sockaddr_in serv_addr;

	int nSocket = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(inet_addr(getIPAddr(hostName)));
	serv_addr.sin_port = htons(serverPort);
	memset(&(serv_addr.sin_zero), 0, 8);

	if(connect(nSocket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
		printf("Error: connect error\n");
	}else{
		//process the message
		if(msgType == 1){ //ADDR
			processADDR(nSocket, data);
		}
		else if(msgType == 2){ //FILESIZE
			processFILESIZE(nSocket, data);
		}
		else if(msgType == 3){ //GET
			processGET(nSocket, data, offset);
		}
		//close and shutdown the socket
		shutdown(nSocket, SHUT_RDWR);
		close(nSocket);
	}

	exit(0);
}
//process ADDR message message
//this send the ADDR request to the host and set the url in the
//data part of the message and wait for ADDR reply from the server
//if it get the reply then print the information otherwise return error
int processADDR(int n_socket, char* url){
	if( url == NULL){
		fprintf(stderr, "Error: host name missing\n");
		return -1;
	}
	ReqMsg request;
	memset(&request, 0, sizeof(ReqMsg));

	request.MsgType = htons(0xFE10);
	request.Offset = 0;
	request.DataLen = strlen(url);
	request.DataLen = htonl(request.DataLen);
	request.Data = url;

	int msg_buf_sz = 10 + request.DataLen;
	unsigned char *msg_buf = (unsigned char*)malloc(msg_buf_sz);
	if (msg_buf == NULL) {
		fprintf(stderr, "Error: malloc() failed\n");
		return -1;
	}
	memset(msg_buf, 0, msg_buf_sz);

	memcpy(msg_buf, &request.MsgType, 2);
	memcpy(msg_buf + 2, &request.Offset, 4);
	memcpy(msg_buf + 6, &request.DataLen, 4);
	memcpy(msg_buf + 10, request.Data, request.DataLen);
	printhex(msg_buf, msg_buf_sz);
	//write the whole data to the socket
	if(write1Byte(n_socket, msg_buf, msg_buf_sz) != msg_buf_sz) {
		fprintf(stderr, "Error: write failed\n");
		free(msg_buf);
		return -1;
	}
	free(msg_buf);

	unsigned char temp_buf[10];
	memset(temp_buf, 0, 10);
	//read the first 10 bytes of the data
	if(read1Byte(n_socket, temp_buf, 10) != 10) {
		fprintf(stderr, "Error: read failed\n");
		return -1;
	}
	printhex(temp_buf, 10);
	memset(&request, 0, sizeof(ReqMsg));

	memcpy(&request.MsgType, temp_buf, 2);
	request.MsgType = ntohs(request.MsgType);
	memcpy(&request.Offset,temp_buf + 2, 4);
	request.Offset = ntohl(request.Offset);
	memcpy(&request.DataLen,temp_buf + 6, 4);
	request.DataLen = ntohl(request.DataLen);
	/*if(request.DataLen > 512){
		fprintf(stderr, "Error: this client can't handle data length more than 512.\n");
		return -1;
	}*/
	if(request.MsgType == 0xFE11){
		msg_buf = (unsigned char*)malloc(request.DataLen + 1);
		if (msg_buf == NULL) {
			fprintf(stderr, "Error: malloc() failed\n");
			return -1;
		}

		if(read1Byte(n_socket, msg_buf, request.DataLen) != request.DataLen) {
			fprintf(stderr, "Error: read failed\n");
			free(msg_buf);
			return -1;
		}
		//printhex(msg_buf, request.DataLen);
		msg_buf[request.DataLen] = '\0';
	}
	if(printmsg == 1){
		printInfo(&request, n_socket);
	}

	if(request.MsgType == 0xFE11){
		printf("\tADDR = %s\n", msg_buf);
		free(msg_buf);
	}
	else if(request.MsgType == 0xFE12){
		printf("\tADDR request for \'%s\' failed.\n", url);
	}
	else if(request.MsgType == 0xFCFE){
		printf("\tADDR request for \'%s\' all failed.\n", url);
	}
	else{
		printf("Message Type can't be recognized.\n");
		return -1;
	}
	return 0;
}
//process FILESIZE message message
//this send the FILESIZE request to the host and set the filename in the
//data part of the message and wait for FILESIZE reply from the server
//if it get the reply then print the information otherwise return error
int processFILESIZE(int n_socket, char* filename){
	/* fill up the request data structure */
	if( filename == NULL){
		fprintf(stderr, "Error: filename missing\n");
		return -1;
	}
	ReqMsg request;
	memset(&request, 0, sizeof(ReqMsg));
	request.MsgType = htons(0xFE20);
	request.Offset = 0;
	request.DataLen = strlen(filename);
	request.DataLen = htonl(request.DataLen);
	request.Data = filename;

	int msg_buf_sz = 10 + request.DataLen;
	unsigned char *msg_buf = (unsigned char*)malloc(msg_buf_sz);
	if (msg_buf == NULL) {
		fprintf(stderr, "malloc() failed\n");
		return -1;
	}
	memset(msg_buf, 0, msg_buf_sz);

	memcpy(msg_buf, &request.MsgType, 2);
	memcpy(msg_buf + 2, &request.Offset, 4);
	memcpy(msg_buf + 6, &request.DataLen, 4);
	memcpy(msg_buf + 10, request.Data, request.DataLen);
	//printhex(msg_buf, msg_buf_sz);
	//write the whole data to the socket
	if(write1Byte(n_socket, msg_buf, msg_buf_sz) != msg_buf_sz) {
		fprintf(stderr, "Error: write failed\n");
		free(msg_buf);
		return (-1);
	}
	free(msg_buf);

	unsigned char temp_buf[10];
	memset(temp_buf, 0, 10);
	//read the first 10 bytes of the data
	if(read1Byte(n_socket, temp_buf, 10) != 10) {
		fprintf(stderr, "Error: read failed\n");
		return (-1);
	}
	//printhex(temp_buf, 10);
	memset(&request, 0, sizeof(ReqMsg));

	memcpy(&request.MsgType, temp_buf, 2);
	request.MsgType = ntohs(request.MsgType);
	memcpy(&request.Offset,temp_buf + 2, 4);
	request.Offset = ntohl(request.Offset);
	memcpy(&request.DataLen,temp_buf + 6, 4);
	request.DataLen = ntohl(request.DataLen);

	/*if(request.DataLen > 512){
		fprintf(stderr, "Error: this client can't handle data length more than 512.\n");
		return -1;
	}*/

	if(request.MsgType == 0xFE21){
		msg_buf = (unsigned char*)malloc(request.DataLen + 1);
		if (msg_buf == NULL) {
			fprintf(stderr, "malloc() failed\n");
			return -1;
		}

		if(read1Byte(n_socket, msg_buf, request.DataLen) != request.DataLen) {
			fprintf(stderr, "Error: read failed\n");
			free(msg_buf);
			return -1;
		}
		//printhex(msg_buf, request.DataLen);
		msg_buf[request.DataLen] = '\0';
	}
	if(printmsg == 1){
		printInfo(&request, n_socket);
	}
	if(request.MsgType == 0xFE21){
		printf("\tFILESIZE = %s\n", msg_buf);
		free(msg_buf);
	}
	else if(request.MsgType == 0xFE22){
		printf("\tFILESIZE request for \'%s\' failed.\n", filename);
	}
	else if(request.MsgType == 0xFCFE){
		printf("\tFILESIZE request for \'%s\' all failed.\n", filename);
	}
	else{
		printf("Message Type can't be recognized.\n");
		return -1;
	}
	return 0;
}

//process GET message message
//this send the GET request to the host and set the filename in the
//data part of the message and wait for GET reply from the server
//if it get the reply then print the information otherwise return error

//if the reply is ok it will then calculate the MD5 of the file data
//on the fly and print it.
int processGET(int n_socket, char* filename,uint32_t off){
	/* fill up the request data structure */
	if( filename == NULL){
		fprintf(stderr, "Error: file name missing\n");
		return -1;
	}
	ReqMsg request;
	memset(&request, 0, sizeof(ReqMsg));
	request.MsgType = htons(0xFE30);
	request.Offset = off;
	request.Offset = htonl(request.Offset);
	request.DataLen = strlen(filename);
	request.DataLen = htonl(request.DataLen);
	request.Data = filename;

	int msg_buf_sz = 10 + request.DataLen;
	unsigned char *msg_buf = (unsigned char*)malloc(msg_buf_sz);
	if (msg_buf == NULL) {
		fprintf(stderr, "malloc() failed\n");
		return -1;
	}
	memset(msg_buf, 0, msg_buf_sz);

	memcpy(msg_buf, &request.MsgType, 2);
	memcpy(msg_buf + 2, &request.Offset, 4);
	memcpy(msg_buf + 6, &request.DataLen, 4);
	memcpy(msg_buf + 10, request.Data, request.DataLen);
	//printhex(msg_buf, msg_buf_sz);
	//write the whole data to the socket
	if(write1Byte(n_socket, msg_buf, msg_buf_sz) != msg_buf_sz) {
		fprintf(stderr, "Error: write failed\n");
		free(msg_buf);
		return -1;
	}
	free(msg_buf);

	unsigned char temp_buf[10];
	memset(temp_buf, 0, 10);
	//read the first 10 bytes of the data
	if(read1Byte(n_socket, temp_buf, 10) != 10) {
		fprintf(stderr, "Error: read failed\n");
		return -1;
	}
	//printhex(temp_buf, 10);
	memset(&request, 0, sizeof(ReqMsg));

	memcpy(&request.MsgType, temp_buf, 2);
	request.MsgType = ntohs(request.MsgType);
	memcpy(&request.Offset,temp_buf + 2, 4);
	request.Offset = ntohl(request.Offset);
	memcpy(&request.DataLen,temp_buf + 6, 4);
	request.DataLen = ntohl(request.DataLen);
	/*if(request.DataLen > 512){
		fprintf(stderr, "Error: this client can't handle data length more than 512.\n");
		return -1;
	}*/

	if(printmsg == 1){
		printInfo(&request, n_socket);
		//printf("\tFILESIZE = %d\n",request.DataLen);
	}
	//calculate MD5 hash of the file data 512 bytes at a time
	unsigned char digest[16];
	memset(digest, 0, 16);
	if(request.MsgType == 0xFE31){
		MD5_CTX ctx;
		MD5_Init(&ctx);

		if(request.DataLen > 0){
			int left = request.DataLen;
			unsigned char fileData[512];
			while(left > 0){
				memset(fileData, 0, 512);
				if(left >= 512){
					//printf("rcv = 512\n");
					if(read1Byte(n_socket, fileData, 512) != 512) {
						fprintf(stderr, "Error: read failed\n");
						return -1;
					}
					//printhex(fileData, 512);
					MD5_Update(&ctx, fileData, 512);
				}
				else{
					//printf("rcv = %d\n", left);
					if(read1Byte(n_socket, fileData, left) != left) {
						fprintf(stderr, "Error: read failed\n");
						return -1;
					}
					//printhex(fileData, left);
					MD5_Update(&ctx, fileData, left);
				}
				left -= 512;
			}
		}
		MD5_Final(digest,&ctx);
	}
	if(request.MsgType == 0xFE31){
		char printdigest[32+1];
		int i = 0;
		for (; i<16; i++){
			sprintf(printdigest+i*2, "%02x", digest[i]);
		}
		printdigest[32] = '\0';
		printf("\tFILESIZE = %d, MD5 = %s\n", request.DataLen, printdigest);
	}
	else if(request.MsgType == 0xFE32){
		printf("\tGET request for \'%s\' failed.\n", filename);
	}
	else if(request.MsgType == 0xFCFE){
		printf("\tGET request for \'%s\' all failed.\n", filename);
	}
	else{
		printf("Message Type can't be recognized.\n");
		return -1;
	}

	return 0;
}
//write one byte at a time into the socket
int write1Byte(int fd, const unsigned char* vptr, int n ){
	int left = n;
	const unsigned char *ptr = vptr;
	int written = 0;
	while(left > 0){
		if((written = write(fd, ptr, 1)) <= 0){
			if(errno == EINTR){
				written = 0;
			}else{
				return -1;
			}
		}
		left--;
		ptr++;
	}
	return n;
}
//read one byte at a time
int read1Byte(int fd, const unsigned char* vptr, int n ){
	int left = n;
	const unsigned char *ptr = vptr;
	int readn = 0;
	while(left > 0){
		if((readn = read(fd, (void*)ptr, 1)) < 0){
			if(errno == EINTR){
				readn = 0;
			}else{
				return -1;
			}
		} else if(readn == 0){
			break; //EOF
		}
		left--;
		if(left != 0) ptr++;
	}
	return (n - left);
}
