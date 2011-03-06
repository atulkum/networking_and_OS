#include "sv_node.h"
/*
This is the main file which contain the main method.It take care of
starting server in a separate thread and shutdown and startups of the 
nodes.
*/

/*Log file name and path
*/
char logfileName[512] = "servant.log";
char logfilePath[512];

/*
These are the property variables which are read from the .ini files.
The default values are aslo assigned so that if some  value is missing in 
.ini file then its default value would be used.
*/
uint16_t autoShutdown = 900;
uint16_t ttl = 30;
uint16_t msgLifeTime = 30;
uint16_t getMsgLifeTime = 300;	
unsigned char initNgbr = 3;// beacon node ignore it
uint16_t joinTimeout = 15; //beacon node ignore it
uint16_t keepalive = 60;
unsigned char minNgbr = 2;// beacon node ignore it
unsigned char noCheck = 0; 
double cacheProb = 0.1;
double storeProb = 0.1;
double ngbrStoreProb = 0.2; 
uint16_t cacheSize = 500;
uint16_t permSize;//obsolete
uint16_t retry = 30;
time_t starttime;
uint16_t myport; 
uint32_t location;
char homeDir[512]; 
/*
Global variables used by the program internally by many files.
These includes flags, linked lists, thread variables, and storage for
global data like hostname.
*/
char myhostname[256];
int isBeacon = 0;
/*
Linked list for storing beacons node information.
*/
ListElement* beaconshead = NULL;
ListElement* beaconstail = NULL;
/*
Linked list for storing neighbor node information.
*/
ListElement* negbhead = NULL;
ListElement* negbtail = NULL;
/*
Linked list for storing router entries information.
*/
ListElement* routerhead = NULL;
ListElement* routertail = NULL;
/*
Linked list for storing connections information.
*/
ListElement* conhead = NULL;
ListElement* contail = NULL;
/*
Mutex to protect routing table access
*/
pthread_mutex_t routerLock;
pthread_t serverThread;
pthread_t inputThread;
char mynodeId[512];
char statusFilename[512];
FILE* logfile;
/*
Mutex to protect log file access
*/
pthread_mutex_t printLock;
char initNgbrPath[512];
int reset = 0;
int isShutdown = 0;
pthread_t timerThread;
int softshutdown = 0;
/*
main Method to run the program. This will first intialize all the 
threads and then try to connect to the neighbor. If it can't get enough 
neighbor it will do a soft restart.
*/
int main(int argc, char *argv[]){
	if(argc < 2){
		printf("\nERROR: ini filename missing\n");
		return -1;			
	}
	char * iniFileName;
	argc--; argv++;	
	int arguments = 1;	
	for(;argc > 0; argc-=arguments, argv += arguments){
		if (!strcmp(*argv, "-reset")) {
			reset = 1;
			arguments = 1;
		}
		else if(strstr(*argv, ".ini")!= NULL){
			iniFileName = *argv;
			arguments = 1;
		}
	}
	//starting the node
	starttime = time(NULL);
	gethostname(myhostname, 256);
	signal(SIGPIPE, sigpipe_handler);	
	sigset_t newsig;
	sigemptyset(&newsig);
	sigaddset(&newsig, SIGINT);
	sigaddset(&newsig, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &newsig, NULL);

	struct sigaction  actions; 
	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = sigusr1_handler;
	sigaction(SIGUSR1,&actions,NULL);

	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = sigalrm_handler;
	sigaction(SIGALRM,&actions,NULL);

	//parse the ini file
	iniparser(iniFileName);	
	if(reset){
		remove(logfilePath);
		remove(initNgbrPath);
	}	
	alarm(autoShutdown);

	while(!isShutdown){
		init();
		//initialization
		pthread_create(&(inputThread), NULL, inputHandler, (void*)NULL);
		pthread_create(&(serverThread), NULL, serverThreadHandler, (void*)NULL);
		
		if(isBeacon){
			ListElement* head = beaconshead;
			while(head != NULL){
				Node* nei = (Node*)head->item;
				//put in connected list
				Append(&conhead, &contail, nei);	
				//start the write thread		
				pthread_create(&(nei->writeThread), NULL, writeHandler, (void*)nei);		
				head = head->next;
			}		
		}
		else{					
			if(!isJoinDone){
				if(dojoin() == -1){
					fprintf( logfile, "** No beaon node found\n");fflush(logfile);					
					isShutdown = 1;
					pthread_kill(inputThread, SIGUSR1);	
				}
				else{					
					softshutdown = 1;					
					pthread_kill(inputThread, SIGUSR1);					
				}
			}
			else{	
				ngbrParser();
				int neededNode = min(initNgbr, minNgbr);			
				ListElement* head = negbhead;
				while(head != NULL){
					Node* nei = (Node*)head->item;					
					//start the write thread		
					if(sendHelloNonBeacon(nei) == 0){
						pthread_create(&(nei->writeThread), NULL, writeHandler, (void*)nei);
						neededNode--;
						Append(&conhead, &contail, nei);	
					}				
					head = head->next;
					if(neededNode == 0){
						break;
					}
				}	
				if(neededNode != 0){
					fprintf( logfile, "** Not enough neighbor found\n");fflush(logfile);
					/*isShutdown = 1;
					pthread_kill(inputThread, SIGUSR1);	*/
					remove(initNgbrPath);
					softshutdown = 1;					
					pthread_kill(inputThread, SIGUSR1);	
				}
			}
		}
		if(softshutdown == 0 &&	isShutdown == 0){
			//pthread_create(&(timerThread), NULL, timerHandler, (void*)NULL);
			pthread_create(&(keepaliveThread), NULL, keepaliveHandler, (void*)NULL);
		}

		int status;
		pthread_join(inputThread, (void*)&status);
		cleanup();	
	}	
	ListElement* head = beaconshead;
	while(head != NULL){					
		Node* other = (Node*)head->item;		
		destroyNode(other);
		head = head->next;
	}
	destroyList(&beaconshead, &beaconstail);		
	exit(0);
}
/*
The initialization method. This will take care of all the intialization requirements.
Like opening a file, Initializing the locks andcondition variables.
*/
void init(){
	softshutdown = 0;
	logfile = fopen (logfilePath, "a+");
	if(logfile == NULL){
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}

	struct stat stFileInfo;
	if(stat(initNgbrPath,&stFileInfo) == 0){
		isJoinDone = 1;
	}
	else{
		isJoinDone = 0;
	}	
	pthread_mutex_init(&(printLock), NULL);
	pthread_mutex_init(&(routerLock), NULL);
	pthread_mutex_init(&(statusDBLock), NULL);
	pthread_mutex_init(&(joinDBLock), NULL);
}
/*
In this method the node try to initate the connection. If not successfull
it will take actions as per the type of nodes (either beacon or regular).
For beacon node it will wait in a loop with retry number of seconds.
For regular node it will give up and says no connection.
*/
void* writeHandler(void *ngbrNode){	
	Node* ngbr = (Node*)ngbrNode;
	if(isBeacon){
		if(sendHelloBeacon(ngbr) == -1){
			pthread_exit(NULL);
		}
	}
	pthread_create(&(ngbr->readThread), NULL, readHandler, (void*)ngbr);
	handleWriteMessage(ngbr);
	pthread_exit(NULL);
}
/*
This is the reader counter part of the writeHandler method. Both the methods would run in 
2 different threads and use the same socket id for writing and reading from the socket.
Both uses blocking system calls for reading and writing.
*/
void* readHandler(void *ngbrNode){	
	Node* ngbr = (Node*)ngbrNode;	
	unsigned char temp_buf[512];
	Message msg;
	memset(temp_buf, 0, 512);
	if(readByte(ngbr->sock_id, temp_buf, 27) != 27){
		//if(ngbr->terminate || isShutdown) {
			closeCon(ngbr);
		//}	
	}			
	readMessage(&msg, temp_buf);	
	if(msg.type == HELLO){
		memset(temp_buf, 0, 512);
		if(readByte(ngbr->sock_id, temp_buf, msg.dataLength) != msg.dataLength){
			//if(ngbr->terminate || isShutdown) {
				closeCon(ngbr);
			//}	
		}
		printlogHello(&msg, temp_buf, ngbr->nodeId, 0);
		//can check simultaneous hello at this point
		Node * other = findNode(ngbr, conhead);
		if((other != NULL) && (other->isHelloDone == 1)){
			if((myport > other->port) || ((myport == other->port) && (strcmp(myhostname,  other->hostname) > 0))){
				//keep this thread and kill the other one
				closeConWrite(other);
			}
			else{
				closeCon(ngbr);
			}
		}
	}
	//no chance of getting a join here
	handleReadMessage(ngbr);
	closeCon(ngbr);
	pthread_exit(NULL);
}
/*
This method is used when some other thread has initiated the connection.
This method first read the connect informationn from the hello packet
and send the hello to the connecting node.
*/
void* readHandlerServer(void *ngbrNode){
	Node* ngbr = (Node*)ngbrNode;
	unsigned char temp_buf[512];
	Message msg;
	memset(temp_buf, 0, 512);
	if(readByte(ngbr->sock_id, temp_buf, 27) != 27){
		//if(ngbr->terminate || isShutdown) {
			closeCon(ngbr);
		//}	
	}	
	readMessage(&msg, temp_buf);

	if(msg.type == HELLO){	
		memset(temp_buf, 0, 512);
		if(readByte(ngbr->sock_id, temp_buf, msg.dataLength) != msg.dataLength){
			//if(ngbr->terminate || isShutdown) {
				closeCon(ngbr);			
			//}	
		}			
		if(processHello(&msg, temp_buf, ngbr) == -1){
			//shutdown and exit			
			shutdown(ngbr->sock_id, SHUT_RDWR);
			close(ngbr->sock_id);
			pthread_exit(NULL);
		}
		ngbr->isHelloDone = 1;		
		pthread_create(&(ngbr->writeThread), NULL, writeHandlerServer, (void*)ngbr);	
	}
	else if(msg.type == JOIN){
		memset(temp_buf, 0, 512);
		if(readByte(ngbr->sock_id, temp_buf, msg.dataLength) != msg.dataLength){
			//if(ngbr->terminate || isShutdown) {
				ngbr->terminate = 1;
				ngbr->isHelloDone = 0;
				
				pthread_cond_signal(&(ngbr->nodeCond));
				pthread_kill(ngbr->readThread, SIGUSR1);			
			
				shutdown(ngbr->sock_id, SHUT_RDWR);
				close(ngbr->sock_id);
				pthread_exit(NULL);
			//}	
		}
		processJoin(&msg, temp_buf, ngbr);
		pthread_create(&(ngbr->writeThread), NULL, writeHandlerJoin, (void*)ngbr);			
		handleJoinRead(ngbr, &msg, temp_buf);											
	}
	handleReadMessage(ngbr);
	closeCon(ngbr);
	pthread_exit(NULL);
}
/*
This is the writer counter part of the readHandlerServer method. Both the methods would run in 
2 different threads and use the same socket id for writing and reading from the socket.
Both uses blocking system calls for reading and writing.
*/
void* writeHandlerServer(void *ngbrNode){
	Node* ngbr = (Node*)ngbrNode;
	Message* hello = (Message*)malloc(sizeof(Message));
	prepareHello(hello);
	if(sendMessage(ngbr->sock_id, hello, ngbr->nodeId) != 0){
		//error etc
	}
	//can check simultaneous hello at this point
	Node * other = findNode(ngbr, conhead);
	if((other != NULL) && (other->isHelloDone == 1) && (other->terminate == 0)){
		if((myport > other->port) || ((myport == other->port) && (strcmp(myhostname,  other->hostname) > 0))){
			//keep this thread and kill the other one
			closeConWrite(other);
		}
		else{
			closeCon(ngbr);
		}
	}
	handleWriteMessage(ngbr);
	pthread_exit(NULL);
}
/*
This method would be used in case of join message exchange as the write thread.
*/
void* writeHandlerJoin(void *ngbrNode){	
	Node* ngbr = (Node*)ngbrNode;
	ngbr->isHelloDone = 2;
	handleWriteMessage(ngbr);
	pthread_exit(NULL);
}
/*
This is the real method which parses packets and take particular actions.
*/

void handleReadMessage(Node* ngb){	
	unsigned char temp_buf[512];
	Message msg;	
	for(;;){
		if(ngb->terminate || isShutdown || softshutdown){
			break;
		}
		memset(temp_buf, 0, 512);
		if(readByte(ngb->sock_id, temp_buf, 27) != 27){
			//if( ngb->terminate || isShutdown) {
				break;
			//}
		}				
		readMessage(&msg, temp_buf);
		if(msg.type == STATUS){
			memset(temp_buf, 0, 512);
			if(readByte(ngb->sock_id, temp_buf, msg.dataLength) != msg.dataLength){
				//if( ngb->terminate || isShutdown) {
					break;
				//}	
			}
			//flood to all other place
			RouterNode* route = findRouterEntry(msg.uoid);
			if((route == NULL) && (msg.ttl > 0) && memcmp(statusuoid, msg.uoid, 20) ){
				putNewRouteEntry(msg.uoid, ngb);
				//if(temp_buf[0] == STATUS_TYPE_NEIGHBORS){
					writeLog(RCV, ngb->nodeId, msg.dataLength, msg.ttl, STRQ, "neighbors", msg.uoid);				
		     		uint32_t length = 20 + 2 + 2 + strlen(myhostname); //uoid + record length + port + hostname
					ListElement* head = conhead;					
					while(head != NULL){					
						Node* nei = (Node*)head->item;
						//if the node has connection and this is not the node from where 
						//the message came then forward it						
						if((nei->isHelloDone == 1) && strcmp(nei->nodeId, ngb->nodeId)){							
							writingSameResMsgttldec(&msg, temp_buf, nei);
						}
						if(nei->isHelloDone == 1){
							length += 2 + 2 + strlen(nei->hostname); //record length + port + hostname
						}
						head = head->next;
					}					
					//send response to the place from where this comes
					sendStatusResponse(1, length, msg.uoid, ngb);					
				//}							
			}
		}
		else if(msg.type == STATUSRESPONSE){			
			memset(temp_buf, 0, 512);
			if(readByte(ngb->sock_id, temp_buf, msg.dataLength) != msg.dataLength){
				//if(ngb->terminate || isShutdown) {
					break;
				//}	
			}
			int8_t uoid2search[20];
			memset(uoid2search, 0, 20);
			memcpy(uoid2search, temp_buf, 20);			
			char toprint[9];
			int2hex((unsigned char*)(uoid2search + 16), 4, toprint);						
			writeLog(RCV, ngb->nodeId, msg.dataLength, msg.ttl, STRS, toprint, msg.uoid);

			//if uoid found then return otherwise it's the originating node
			RouterNode* route = findRouterEntry(uoid2search);			
			if(route != NULL){		
				//just pass the whole packet
				writingSameResMsg(&msg, temp_buf, route->tosend);
			}
			else{				
				//store somewhere
				if(!memcmp(statusuoid, uoid2search, 20)){				
					storeStatusInfoNgbr(msg.dataLength, temp_buf);
				}
				else{
					//message time expire
				}
			}
		}
		if(msg.type == CHECK){
			memset(temp_buf, 0, 512);
			if(readByte(ngb->sock_id, temp_buf, msg.dataLength) != msg.dataLength){
				//if( ngb->terminate || isShutdown) {
					break;
				//}	
			}
			//flood to all other place
			RouterNode* route = findRouterEntry(msg.uoid);
			if((route == NULL) && (msg.ttl > 0)&& memcmp(checkuoid, msg.uoid, 20)){
				putNewRouteEntry(msg.uoid, ngb);				
				writeLog(RCV, ngb->nodeId, msg.dataLength, msg.ttl, CKRQ, "", msg.uoid);				
				if(!isBeacon){
					ListElement* head = conhead;					
					while(head != NULL){					
						Node* nei = (Node*)head->item;
						//if the node has connection and this is not the node from where 
						//the message came then forward it						
						if((nei->isHelloDone == 1) && strcmp(nei->nodeId, ngb->nodeId)){							
							writingSameResMsgttldec(&msg, temp_buf, ngb);
						}
						head = head->next;
					}
				}
				else{
					//send response to the place from where this comes
					sendCheckResponse(msg.uoid, ngb);
				}
			}
		}
		else if(msg.type == CHECKRESPONSE){			
			memset(temp_buf, 0, 512);
			if(readByte(ngb->sock_id, temp_buf, msg.dataLength) != msg.dataLength){
				//if(ngb->terminate || isShutdown) {
					break;
				//}	
			}
			int8_t uoid2search[20];
			memset(uoid2search, 0, 20);
			memcpy(uoid2search, temp_buf, 20);			
			char toprint[9];
			int2hex((unsigned char*)(uoid2search + 16), 4, toprint);				
			writeLog(RCV, ngb->nodeId, msg.dataLength, msg.ttl, CKRS, toprint, msg.uoid);

			//if uoid found then return otherwise it's the originating node
			RouterNode* route = findRouterEntry(uoid2search);			
			if(route != NULL){		
				//just pass the whole packet
				writingSameResMsg(&msg, temp_buf, route->tosend);
			}
			else{				
				//store somewhere
				if(!memcmp(checkuoid, uoid2search, 20)){				
					isCheckOk = 1;
				}
				else{
					//message time expire
				}
			}
		}
		else if(msg.type == KEEPALIVE){
			writeLog(RCV, ngb->nodeId, msg.dataLength, msg.ttl, KPAV, "", msg.uoid);	
			ngb->kplvLifeTime = keepalive;
		}		
		else if(msg.type == JOIN){
			memset(temp_buf, 0, 512);
			if(readByte(ngb->sock_id, temp_buf, msg.dataLength) != msg.dataLength){
				//if(ngb->terminate || isShutdown) {
					break;
				//}			
			}									
			//flood to all other place
			RouterNode* route = findRouterEntry(msg.uoid);
			if((route == NULL) && (msg.ttl > 0) && memcmp(joinuoid, msg.uoid, 20)){
				printJoin(&msg, temp_buf, ngb->nodeId, 2);
				handleJoinRead(ngb, &msg, temp_buf);
			}
			else{
				printJoin(&msg, temp_buf, ngb->nodeId, 0);
			}
		}
		else if(msg.type == JOINRESPONSE){
			memset(temp_buf, 0, 512);
			if(readByte(ngb->sock_id, temp_buf, msg.dataLength) != msg.dataLength){
				//if(ngb->terminate || isShutdown) {
					break;
				//}		
			}						
			int8_t uoid2search[20];			
			memset(uoid2search, 0, 20);
			memcpy(uoid2search, temp_buf, 20); 
			//if uoid found then pass 
			RouterNode* route = findRouterEntry(uoid2search);	
			if(route != NULL){		
				//just pass the whole packet
				printJoinResponse(&msg, temp_buf, ngb->nodeId, 2);
				writingSameResMsg( &msg, temp_buf, route->tosend);
			}
			else{
				//dropping
				printJoinResponse(&msg, temp_buf, ngb->nodeId, 0);
			}
		}
		else if(msg.type == NOTIFY){
			writeLog(RCV, ngb->nodeId, msg.dataLength, msg.ttl, NTFY, "1", msg.uoid); //for now hardcoded									
			ngb->terminate = 1;
			ngb->isHelloDone = 0;
			closeCon(ngb);
		}
	}
}
/*
This method is the main method which take care of the write part.
It will  make the packet and enqueue them into the writer thread queue.
the writer thread queue will pick the message and send them to the 
correcponding node.
*/

void handleWriteMessage(Node* ngb){		
	for(;;){
		if(ngb->terminate || isShutdown || softshutdown){
			break;
		}
		pthread_mutex_lock(&ngb->nodeLock);
		pthread_cond_wait(&ngb->nodeCond, &ngb->nodeLock);		
		if(ngb->terminate || isShutdown || softshutdown){
			break;
		}
		else{
			//get the message from the queue and send it
			while(ngb->qlen){			
				Message* msg = (Message*)Remove(&(ngb->writeQhead), &(ngb->writeQtail));
				ngb->qlen--;
				pthread_mutex_unlock(&ngb->nodeLock);					
				sendMessage(ngb->sock_id, msg, ngb->nodeId);			
			}
		}
	}	
	while(ngb->qlen){			
		Message* msg = (Message*)Remove(&(ngb->writeQhead), &(ngb->writeQtail));
		ngb->qlen--;
		pthread_mutex_unlock(&ngb->nodeLock);					
		if(msg->dataLength > 0){
			free(msg->data);
		}
		free(msg);
	}	
}
/*
This is the main server method where a server is keep waiting for the connection
on a well known port.
*/
void *serverThreadHandler(void *dummy){
	//start listening on well known port
	struct sockaddr_in serv_addr;
	int nSocket = socket(AF_INET, SOCK_STREAM, 0);	
	int reuse_addr = 1;
	if (setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (void*)(&reuse_addr), sizeof(int)) == -1) {
		fprintf(logfile, "** Error: setsockopt failed\n"); fflush(logfile);
    }
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(myport);
	memset(&(serv_addr.sin_zero), 0, 8);
	
	if(bind(nSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
		fprintf(logfile, "** Error: bind failed\n");fflush(logfile);		
		isShutdown = 1;		
		pthread_kill(inputThread, SIGUSR1);	
		pthread_exit(NULL);
	}
	if(listen(nSocket, 5) == -1){
		fprintf(logfile, "** Error: listen failed\n");fflush(logfile);		
		isShutdown = 1;		
		pthread_kill(inputThread, SIGUSR1);	
		pthread_exit(NULL);
	}		
	//printf("server1\n");
	for(;;){
		//printf("server2\n");
		if(isShutdown || softshutdown){
			break;
		}
		struct sockaddr_in cli_addr;
		int clilen=sizeof(cli_addr), newsockfd = 0;
		newsockfd = accept(nSocket, (struct sockaddr *)(&cli_addr), &clilen);
		if(newsockfd < 0){
			//if(isShutdown || softshutdown){
				break;
			//}
			//printf("server3\n");
		}
		else{
			Node* other = createEmptyNode();
			other->istemp = 1;
			Append(&conhead, &contail, other);
			other->sock_id = newsockfd;
			pthread_create(&(other->readThread), NULL, readHandlerServer, (void*)other);
		}
	}	
	//printf("server4\n");
	shutdown(nSocket, SHUT_RDWR);
	close(nSocket);
	pthread_exit(NULL);
}
/*
This method will take care of the command line input. Currently
it is handling two commands:
status: This command will produce the network topology in nam format.
shutdown: this command initiate the shutdown of the system.
*/
void* inputHandler(void * dummy){
	sigset_t newsig;
	sigemptyset(&newsig);
	sigaddset(&newsig, SIGINT);
	sigaddset(&newsig, SIGTERM);

	struct sigaction act;
	act.sa_handler = interrupt;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	pthread_sigmask(SIG_UNBLOCK, &newsig, NULL);

	char string [256];
	do{
		if(isShutdown || softshutdown){
			break;
		}
		printf ("\nservant:%d>", myport);
		char* temp = gets(string);

		if(isShutdown || softshutdown){
			break;
		}
		if(temp == NULL || !strcmp(string, "")){
			continue;
		}	
		char *token;
		token = strtok(string, " ");
		if(!strcmp(token, "shutdown")){
			isShutdown = 1;
			break;
		}
		else if(!strcmp(token, "status")){
			if(isStatusOn){
				continue;
			}
			int isnbr = 0;
			token = strtok(NULL, " ");
			if(!strcmp(token, "neighbors")){
				isnbr = 1;
			}
			else if(!strcmp(token, "files")){
				isnbr = 0;
			}
			else{
				//error
			}
			token = strtok(NULL, " ");
			uint16_t passedttl =  atoi(token);

			token = strtok(NULL, " ");
			memset(statusFilename, 0, 512);
			strcpy(statusFilename, token);
							
			if(passedttl == 0){
				//write the information about the node 
				FILE* file = fopen (statusFilename, "w");
				if(file == NULL){
					fprintf(logfile, "** ERROR: status output file open fails\n");fflush(logfile);
				}							
				fprintf(file, "V -t * -v 1.0a5\n");	
				fprintf(file, "n -t * -s 1 -c red -i black\n");					
				fclose(file);
			}
			else{
				isStatusOn = 1;
				pthread_mutex_lock(&(statusDBLock));				
				statusDBlen = 0;							
				pthread_mutex_unlock(&(statusDBLock));				
				char inst_id[512];	
				int8_t uoid[20];
				memset(inst_id, 0, 512);	
				memset(uoid, 0, 20);
				getNodeInstId(inst_id, mynodeId);
				GetUOID(inst_id, "msg", uoid, 20);
				memset(statusuoid, 0, 20);
				memcpy(statusuoid, uoid, 20);
				ListElement* head = conhead;
				while(head != NULL){					
					Node* nei = (Node*)head->item;
					if(nei->isHelloDone == 1){
						Message * status = (Message*)malloc(sizeof(Message));
						if (status == NULL) {
							fprintf(logfile, "** malloc() failed\n");
							fflush(logfile);
						}
						prepareStatusWithUoid(status, passedttl, isnbr, uoid);						
						//if(status->data[0] == 1){
							writeLog(SEND, nei->nodeId, status->dataLength, status->ttl, STRQ, "neighbors", status->uoid);
						/*}
						else{
							writeLog(SEND, nei->nodeId, status->dataLength, status->ttl, STRQ, "files", status->uoid);
						}*/
					
						pthread_mutex_lock(&(nei->nodeLock));				
						Append(&(nei->writeQhead), &(nei->writeQtail), status);
						nei->qlen++;
						pthread_mutex_unlock(&(nei->nodeLock));
						pthread_cond_signal(&(nei->nodeCond));			
					}
					head = head->next;
				}				
				//start a new thread which will wait for msglifetime + 5
				//and after that it will set status done and process the status message
				//and write into a file
				pthread_create(&(statusThread), NULL, statusHandler, (void*)NULL);
			}
		}
	}while(strcasecmp(string, "shutdown"));	
	pthread_exit(NULL);
}
/*
Cleanup metho. This is callled before the system is shutting down.
It clean every allocated memory, destry mutex and condition variables,
and send signal to the thread for finishing up.
*/
void cleanup(){	
	int status;
//printf("trap2\n");
	pthread_kill(serverThread, SIGUSR1);	
	pthread_join(serverThread, (void*)&status);
//printf("trap3\n");
	pthread_kill(statusThread, SIGUSR1);	
	pthread_join(statusThread, (void*)&status);
//printf("trap4\n");
	pthread_kill(joinThread, SIGUSR1);	
	pthread_join(joinThread, (void*)&status);
//printf("trap5\n");
	pthread_kill(checkThread, SIGUSR1);	
	pthread_join(checkThread, (void*)&status);
//printf("trap6\n");
	pthread_kill(timerThread, SIGUSR1);	
	pthread_join(timerThread, (void*)&status);
//printf("trap7\n");
//destroy all locks and cond in nodes
	ListElement* head = conhead;	
	while(head != NULL){					
		Node* other = (Node*)head->item;
		if(other->isHelloDone == 1 && other->terminate == 0){
//printf("trap8\n");
			closeConWrite(other);	
			pthread_join(other->readThread, (void*)&status);
			pthread_join(other->writeThread, (void*)&status);
		}
		if(other->isHelloDone == 2 && other->terminate == 0){			
			other->terminate = 1;
			other->isHelloDone = 0;
			pthread_cond_signal(&(other->nodeCond));	
			pthread_kill(other->readThread, SIGUSR1);	

			pthread_join(other->readThread, (void*)&status);
			pthread_join(other->writeThread, (void*)&status);
		}
		if(other->istemp){
			destroyNode(other);
		}
		head = head->next;
	}

	head = negbhead;
	while(head != NULL){					
		Node* other = (Node*)head->item;		
		destroyNode(other);
		head = head->next;
	}
	destroyList(&conhead, &contail);	
	destroyList(&negbhead, &negbtail);

	pthread_mutex_destroy(&printLock);
	pthread_mutex_destroy(&routerLock);
	pthread_mutex_destroy(&statusDBLock);
	pthread_mutex_destroy(&joinDBLock);
	fclose(logfile);	
}
/*
This methos is used for disconnecting a node.
This will terminate both the write and the read thread.
*/
void closeConWrite(Node* nei){
	if(nei->isHelloDone == 1){
		Message* notify = (Message*)malloc(sizeof(Message));
		prepareNotify(notify);
		sendMessage(nei->sock_id, notify, nei->nodeId);			
	}	
	nei->terminate = 1;
	nei->isHelloDone = 0;
	pthread_cond_signal(&(nei->nodeCond));	
	pthread_kill(nei->readThread, SIGUSR1);	
}

/*
This methos is used for disconnecting a node.
This will only terminate the write thread.
*/
void closeCon(Node* nei){
	if(nei->isHelloDone == 1){
		Message* notify = (Message*)malloc(sizeof(Message));
		prepareNotify(notify);
		sendMessage(nei->sock_id, notify, nei->nodeId);			
	}
	nei->terminate = 1;
	nei->isHelloDone = 0;
	pthread_cond_signal(&(nei->nodeCond));			
	shutdown(nei->sock_id, SHUT_RDWR);
	close(nei->sock_id);
	doCheck();
	pthread_exit(NULL);
}
/*
This method is called by the timer. It will reduce the keepalive time 
of the connection.
*/
void reducekeepalive(){
	ListElement* head = conhead;
	while(head != NULL){				
		Node* ngb = (Node*)head->item;
		//printf("kplv %s\n" , ngb->nodeId);
		if(ngb->isHelloDone == 1 || ngb->terminate == 0){
			ngb->kplvLifeTime--;
			//if(ngb->kplvLifeTime == 0){
				//closeConWrite(ngb);
			//}
		}
		head = head->next;
	}	
}
/*
This is the thread routine for handling timer.
*/
void* timerHandler(void* dummy){
	for(;;){		
		if(isShutdown || softshutdown){
			break;
		}
		struct timeval delay;
		delay.tv_sec = 1;
		delay.tv_usec = 0;
		select(0, NULL, NULL, NULL, &delay);		
		if(isShutdown || softshutdown){
			break;
		}		
		reduceLifeTime();
		reducekeepalive();
	}

	pthread_mutex_lock(&routerLock);
	ListElement* head = routerhead;
	while(head != NULL){					
		RouterNode* other = (RouterNode*)head->item;		
		free(other);
		head = head->next;
	}
	destroyList(&routerhead, &routertail);	
	pthread_mutex_unlock(&routerLock);

	pthread_exit(NULL);
}
