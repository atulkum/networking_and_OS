#include "server.h"

unsigned char printmsg = 0; //0 = don't print message , 1 = print message
unsigned char isShutdown = 0; //0 = don't shutdown , 1 = shutdown
int childpid[64]; //array to store the child pid
int delay = 0; //delay in sending reply
int childpidptr = 0;
//signla handler for SIGALRM
void sigalrm_handler(int sig_num){
	isShutdown = 1;
}
//signla handler for SIGINT
void sigint_handler(int sig_num){
	signal(SIGINT, SIG_IGN);
	isShutdown = 1;
}
//signla handler for SIGUSR1
void sigusr1_handler(int sig_num){
	signal(SIGUSR1, SIG_IGN);
	isShutdown = 1;
}
//signla handler for SIGCHLD
void sigchld_handler (int signum){
	//set the SIGCHLD handler so that the handler is called
	//next time also
	signal(SIGCHLD, sigchld_handler);
/*	int pid;
	int status;
	for(;;){
		pid = wait (&status);
		if (pid < 0){
			//perror ("waitpid");
			break;
		}
		if (pid == 0){
			break;
		}
		//mask the signals
		sigset_t mask_set;
		sigset_t old_set;
		sigfillset(&mask_set);
		sigprocmask(SIG_SETMASK, &mask_set, &old_set);
		int id = 0;
		//remove the pid from the child pid array
		for(; id < 64; ++id){
			if(childpid[id] == pid){
				childpid[id] = 0;
			}
		}
		//unmask the signals
		sigprocmask(SIG_SETMASK, &old_set, NULL);
	}*/
}
//main method. It parses the command line argument passed and
//starts the server
int main(int argc, char *argv[]){
	memset(childpid, 0, 64);
	//server port
	uint16_t serverPort = 0;
	//time out seconds, by default set to 60 seconds
	int seconds = 60;
	//Required arguments missing.
	if(argc < 2){
		fprintf(stderr, "Error: Required arguments missing.\n");
		exit(0);
	}
	//escaping the executable name
	argc--; argv++;
	//the number of arguments consumed in one iteration
	int arguments = 1;
	//this for loop go through all the command line argument and set the various values
	for(;argc > 0; argc-=arguments, argv += arguments){
		if (!strcmp(*argv, "-m")) {
			printmsg = 1;
			arguments = 1;
		}
		else if(!strcmp(*argv, "-t")){
			if(argc == 1){
				fprintf(stderr, "Error: malformed command.\n");
				exit(0);
			}
			if(isalpha(*(*(argv + 1)))){
				fprintf(stderr, "Error: bad timeout argument.\n");
				exit(0);
			}
			seconds = atoi(*(argv + 1));
			arguments = 2;
		}
		else if(!strcmp(*argv, "-d")){
			if(argc == 1){
				fprintf(stderr, "Error: malformed command.\n");
				exit(0);
			}
			if(isalpha(*(*(argv + 1)))){
				fprintf(stderr, "Error: bad delay argument.\n");
				exit(0);
			}
			delay = atoi(*(argv + 1));
			arguments = 2;
		}
		else if(serverPort == 0){
			if(isalpha(*(*argv)) ){
				fprintf(stderr, "Error: bad port value.\n");
				exit(0);
			}
			serverPort = atoi(*argv);
			arguments = 1;
		}
		else{
			fprintf(stderr, "Error: malformed command.\n");
			exit(0);
		}
	}
	//we are assuming that we don't have priviledge to run as
	//root user so resreved port number is not allowed
	if(serverPort < 1024 ){
		fprintf(stderr, "Error: port value is reserved.\n");
		exit(0);
	}
	//set signal handlers
	signal(SIGALRM, sigalrm_handler);
	signal(SIGINT, sigint_handler);
	signal(SIGCHLD, sigchld_handler);
	//preapre the sockaddr structure
	struct sockaddr_in serv_addr;
	int nSocket = 0;
	nSocket = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(serverPort);
	memset(&(serv_addr.sin_zero), 0, 8);
	//bind the port
	if(bind(nSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
		fprintf(stderr, "Error: bind failed\n");
		return -1;
	}
	//this is to make sure that port is available for reuse after server shutdown
	int reuse_addr = 1;
	if (setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (void*)(&reuse_addr), sizeof(int)) == -1) {
		fprintf(stderr, "Error: setsockopt failed\n");
    }
    //listen with the number of outstanding connections in the socket's listen queue is 5
	if(listen(nSocket, 5) == -1){
		fprintf(stderr, "Error: listen failed\n");
		return -1;
	}
	//set the alarm for specified seconds
	alarm(seconds);
	//wait for a connection in an infinite loop
	for(;;){
		//if shutdown is requested then com eout of the loop
		if(isShutdown == 1) {
			break;
		}
		//sockaddr structure for client
		struct sockaddr_in cli_addr;
		int clilen=sizeof(cli_addr), newsockfd = 0;
		//accept a client connection
		newsockfd = accept(nSocket, (struct sockaddr *)(&cli_addr), &clilen);
		//returned a bad socket
		if(newsockfd < 0){
			//check for interrupt
			if(errno == EINTR) {

			}
		}
		else{
			//client is ready to communicate. fork a new child to handle this client
			int pid = fork();
			if(pid < 0){
				fprintf(stderr, "Error: fork failed\n");
			}
			else if(pid == 0){
				//inside child process
				//close the parent socket, child don't need it
				close(nSocket);
				//set the SIGUSR1 signal handler
				signal(SIGUSR1, sigusr1_handler);
				//process the message
				processMsg(newsockfd);
				//processing done, shutdown and close the client socket, and exit after that.
				shutdown(newsockfd, SHUT_RDWR);
				close(newsockfd);
				exit(0);
			}
			else{
				//inside parent process
				//close child socket, parent doesn't need it
				close(newsockfd);
				//mask the signal
				sigset_t mask_set;
				sigset_t old_set;
				sigfillset(&mask_set);
				sigprocmask(SIG_SETMASK, &mask_set, &old_set);

				//store the created pid in the child pid array
				childpid[childpidptr++] = pid;

				//unmask the signals
				sigprocmask(SIG_SETMASK, &old_set, NULL);
			}
		}
	}
	//gracefully shutdown and exit
	//shutdown and close the server socket
	shutdown(nSocket, SHUT_RDWR);
	close(nSocket);

	printf("Server shutting down...\n");
	//ignore the SIGCHLD, it is done to get rid of zombie process
	int id = 0;
	int status;
	//send all processing childs kill signal to finish and wait for them to finish
	for(; id < childpidptr; ++id){
		kill(childpid[id], SIGUSR1);
		waitpid (childpid[id], &status, 0);
	}
	signal(SIGCHLD, SIG_IGN);
   	exit(0);
}
//routine for processing message header
int processMsg(int n_socket){
	unsigned char temp_buf[10];
	memset(temp_buf, 0, 10);

	//read the first 10 bytes of the data
	if(read1Byte(n_socket, temp_buf, 10) != 10) {
		fprintf(stderr, "Error: read failed ending transfer\n");
		processALLFAIL(n_socket);
		return (-1);
	}

	printhex(temp_buf, 10);
	ReqMsg request;
	memset(&request, 0, sizeof(ReqMsg));
	//set the request structure
	memcpy(&request.MsgType, temp_buf, 2);
	request.MsgType = ntohs(request.MsgType);
	memcpy(&request.Offset,temp_buf + 2, 4);
	request.Offset = ntohl(request.Offset);
	memcpy(&request.DataLen,temp_buf + 6, 4);
	request.DataLen = ntohl(request.DataLen);
	if(printmsg == 1){
		printInfoServer(&request, n_socket);
	}
	//if data length is longer than 512 send all fail message
	if(request.DataLen > 512){
		fprintf(stderr, "Error: this server can't handle data length more than 512.\n");
		processALLFAIL(n_socket);
		return -1;
	}
	//if delay in reply is requested then wait for delay seconds
	if(delay > 0){
		sleep(delay);
	}
	//process the various kind of messages
	if(request.MsgType == 0xFE10){
		processADDR(n_socket, &request);
	}
	else if(request.MsgType == 0xFE20){
		processFILESIZE(n_socket, &request);
	}
	else if(request.MsgType == 0xFE30){
		processGET(n_socket, &request);
	}
	else {
		//no valid message send all fail
		printf("Message Type can't be recognized.\n");
		processALLFAIL(n_socket);
	}
	return 0;
}
//send ALL FAIL message
int processALLFAIL(int n_socket){
	printf("Sending all fail.\n");
	ReqMsg req;
	memset(&req, 0, sizeof(ReqMsg));

	req.MsgType = htons(0xFCFE);

	int msg_buf_sz = 10;
	unsigned char* msg_buf = (unsigned char*)malloc(msg_buf_sz);

	if (msg_buf == NULL) {
		fprintf(stderr, "malloc() failed\n");
		return -1;
	}
	//set the buffer with information
	memset(msg_buf, 0, msg_buf_sz);
	memcpy(msg_buf, &req.MsgType, 2);

	//write the whole data to the socket
	if(write1Byte(n_socket, msg_buf, msg_buf_sz) != msg_buf_sz) {
		fprintf(stderr, "Error: write failed ending transfer\n");
		free(msg_buf);
		return (-1);
	}
	free(msg_buf);
	return 0;
}
//routine for processing ADDR type message
int processADDR(int n_socket, ReqMsg* req){
	unsigned char* msg_buf = (unsigned char*)malloc(req->DataLen + 1);
	if (msg_buf == NULL) {
		fprintf(stderr, "malloc() failed\n");
		return -1;
	}
	//read the data part
	if(read1Byte(n_socket, msg_buf, req->DataLen) != req->DataLen) {
		fprintf(stderr, "Error: read failed ending transfer\n");
		free(msg_buf);
		return (-1);
	}
	printhex(msg_buf, req->DataLen);
	msg_buf[req->DataLen] = '\0';

	//printf("\tADDR = %s\n", msg_buf);

	//get the dotted ip address
	const char* ipaddr = getIPAddr((char*)msg_buf);

	memset(req, 0, sizeof(ReqMsg));
	if(ipaddr == NULL){
		//if ip address is not right send ADDR_FAIL message
		req->MsgType = htons(0xFE12);
	}
	else{
		req->MsgType = htons(0xFE11);
		req->Offset = 0;
		req->DataLen = strlen(ipaddr);
		req->DataLen = htonl(req->DataLen);
	}

	int msg_buf_sz = 10 + req->DataLen;
	unsigned char* send_buf = (unsigned char*)malloc(msg_buf_sz);
	if (send_buf == NULL) {
		fprintf(stderr, "malloc() failed\n");
		free(msg_buf);
		return -1;
	}
	//set the send buffer
	memset(send_buf, 0, msg_buf_sz);
	memcpy(send_buf, &req->MsgType, 2);
	memcpy(send_buf + 2, &req->Offset, 4);
	memcpy(send_buf + 6, &req->DataLen, 4);
	if(ipaddr != NULL){
		memcpy(send_buf + 10, ipaddr, req->DataLen);
	}
	free(msg_buf);
	printhex(send_buf, msg_buf_sz);
	//write the whole data to the socket
	if(write1Byte(n_socket, send_buf, msg_buf_sz) != msg_buf_sz) {
		fprintf(stderr, "Error: write failed ending transfer\n");
		free(send_buf);
		return (-1);
	}
	free(send_buf);
	return 0;
}
int processFILESIZE(int n_socket, ReqMsg* req){
	unsigned char* msg_buf = (unsigned char*)malloc(req->DataLen + 1);
	if (msg_buf == NULL) {
		fprintf(stderr, "malloc() failed\n");
		return -1;
	}
	//read the file name
	if(read1Byte(n_socket, msg_buf, req->DataLen) != req->DataLen) {
		fprintf(stderr, "Error: read failed ending transfer\n");
		free(msg_buf);
		return (-1);
	}
	//printhex(msg_buf, req->DataLen);
	msg_buf[req->DataLen] = '\0';

	//printf("\tFILESIZE = %s\n", msg_buf);

	//get the dotted ip address
	int size = getFileSize(msg_buf);
	free(msg_buf);
	char temp[256];
	memset(req, 0, sizeof(ReqMsg));

	if(size == -1){
		//if file is not accessible send an FILESIZE_FAIL message
		req->MsgType = htons(0xFE22);
		req->DataLen = 0;
	}
	else{
		sprintf(temp,"%d", size);
		req->MsgType = htons(0xFE21);
		req->DataLen = strlen(temp);
		req->DataLen = htonl(req->DataLen);
	}
	req->Offset = 0;
	//send the FILESIZE_REPLY message
	int msg_buf_sz = 10 + req->DataLen;
	msg_buf = (unsigned char*)malloc(msg_buf_sz);
	if (msg_buf == NULL) {
		fprintf(stderr, "malloc() failed\n");
		return -1;
	}

	memset(msg_buf, 0, msg_buf_sz);
	memcpy(msg_buf, &req->MsgType, 2);
	memcpy(msg_buf + 2, &req->Offset, 4);
	memcpy(msg_buf + 6, &req->DataLen, 4);
	if(size != -1){
		memcpy(msg_buf + 10, temp, req->DataLen);
	}

	printhex(msg_buf, msg_buf_sz);
	//write the whole data to the socket
	if(write1Byte(n_socket, msg_buf, msg_buf_sz) != msg_buf_sz) {
		fprintf(stderr, "Error: write failed ending transfer\n");
		free(msg_buf);
		return (-1);
	}
	free(msg_buf);
	return 0;
}
int processGET(int n_socket, ReqMsg* req){
	unsigned char* msg_buf = (unsigned char*)malloc(req->DataLen + 1);
	if (msg_buf == NULL) {
		fprintf(stderr, "malloc() failed\n");
		return -1;
	}
	//read the file name
	if(read1Byte(n_socket, msg_buf, req->DataLen) != req->DataLen) {
		fprintf(stderr, "Error: read failed ending transfer\n");
		free(msg_buf);
		return (-1);
	}

	printhex(msg_buf, req->DataLen);
	msg_buf[req->DataLen] = '\0';

	//printf("\tGET = %s\n", msg_buf);

	int fail = 0;
	int fd;
	if ( (fd = open((const char*)msg_buf,O_RDONLY)) == -1){
		fprintf(stderr, "Error: file open failed\n");
		fail = 1;
	}
	free(msg_buf);

	off_t start, end;

	if(fail != 1){
		//calculate the file size
		start = lseek(fd, req->Offset, SEEK_SET);
		end = lseek(fd, 0, SEEK_END);
		if(start == -1 || (end - start) == 0){ //out of the file or at the end of the file
			fprintf(stderr, "Error: lseek failed\n");
			fail = 2;
		}
	}

	memset(req, 0, sizeof(ReqMsg));

	if(fail == 1){
		//fine can't be open send the fail message
		req->MsgType = htons(0xFE32);
		req->Offset = 0;
		req->DataLen = 0;

		unsigned char send_buf[10];
		memset(send_buf, 0, 10);

		memcpy(send_buf, &req->MsgType, 2);
		memcpy(send_buf + 2, &req->Offset, 4);
		memcpy(send_buf + 6, &req->DataLen, 4);

		printhex(send_buf, 10);
		//write the whole data to the socket
		if(write1Byte(n_socket, send_buf, 10) != 10) {
			fprintf(stderr, "Error: write failed ending transfer\n");
			close(fd);
			return (-1);
		}
	}
	else if(fail == 2){
		//the offset is out of or at the end of file boundary send zero length
		req->MsgType = htons(0xFE31);
		req->Offset = 0;
		req->DataLen = 0;

		unsigned char send_buf[10];

		memset(send_buf, 0, 10);

		memcpy(send_buf, &req->MsgType, 2);
		memcpy(send_buf + 2, &req->Offset, 4);
		memcpy(send_buf + 6, &req->DataLen, 4);

		printhex(send_buf, 10);
		//write the whole data to the socket
		if(write1Byte(n_socket, send_buf, 10) != 10) {
			fprintf(stderr, "Error: write failed ending transfer\n");
			close(fd);
			return (-1);
		}
	}
	else{
		//send the file data
		req->MsgType = htons(0xFE31);
		req->Offset = 0;
		req->DataLen = end - start;
		//printf("filesize = %d \n", req->DataLen);
		unsigned char send_buf[10];

		memset(send_buf, 0, 10);

		memcpy(send_buf, &req->MsgType, 2);
		memcpy(send_buf + 2, &req->Offset, 4);
		memcpy(send_buf + 6, &req->DataLen, 4);

		printhex(send_buf, 10);
		//first write the header to the socket
		if(write1Byte(n_socket, send_buf, 10) != 10) {
			fprintf(stderr, "Error: write failed ending transfer\n");
			close(fd);
			return (-1);
		}

		lseek(fd, start, SEEK_SET);

		//now send the file bytes in 512 bytes chunk at a time
		unsigned char fileData[512];
		off_t bytesTosend;
		while(1){
			memset(fileData, 0, 512);
			bytesTosend = read(fd, fileData, 512);
			if(bytesTosend > 0){
				//printhex(fileData, 512);
				if(write1Byte(n_socket, fileData, bytesTosend) != bytesTosend) {
					fprintf(stderr, "Error: write failed ending transfer\n");
					close(fd);
					return (-1);
				}
				//printf("bytesTosend = %d \n", bytesTosend);
			}
			else{
				break;
			}
		}
	}
	close(fd);
	return 0;
}
//write one byte at a time into the socket
int write1Byte(int fd, const unsigned char* vptr, int n ){
	int left = n;
	const unsigned char *ptr = vptr;
	int written = 0;
	while(left > 0){
		if(isShutdown == 1){
			//if shutdown requested return
			return -1;
		}
		//write one byte
		if((written = write(fd, ptr, 1)) <= 0){
			if(errno == EINTR){
				written = 0;
			}else{
				//if write fails return
				return (-1);
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
		if(isShutdown == 1){
			//if shutdown requested return
			return -1;
		}
		//read one byte from the socket
		if((readn = read(fd, (void*)ptr, 1)) < 0){
			if(errno == EINTR){
				readn = 0;
			}else{
				//if read fails return error
				return -1;
			}
		} else if(readn == 0){
			break; //EOF
		}
		left--;
		if(left != 0) ptr++;
	}
	//return the bytes end
	return (n - left);
}
