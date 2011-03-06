#include "common.h"
/*
This file contain common utility functions used by all other methods.
*/
char msgdir[][3] = {"r", "f", "s", "**", "//"};
char msgtype[][5] = {"NONE", "JNRQ", "JNRS", "HLLO","KPAV", "NTFY", "CKRQ", "CKRS", "SHRQ", "SHRS", "GTRQ", "GTRS", "STOR", "DELT", "STRQ", "STRS"};
KwInfo gkwinfo[] = {
	{ KW_PORT, "Port",1 },
	{ KW_LOCATION, "Location",1 },
	{ KW_HOMEDIR, "HomeDir",1 },
	{ KW_LOGFILE, "LogFilename",0},
	{ KW_AUTOSHUTDOWN, "AutoShutdown",0},
	{ KW_TTL, "TTL",0},
	{ KW_MSGLIFETIME, "MsgLifetime",0},
	{ KW_GETMSGLIFETIME, "GetMsgLifetime",0},
	{ KW_INITNGBR, "InitNeighbors",0},
	{ KW_JOINTIMEOUT, "JoinTimeout",0},
	{ KW_KEEPALIVE, "KeepAliveTimeout",0},
	{ KW_MINNGBR, "MinNeighbors",0},
	{ KW_NOCHECK, "NoCheck",0},
	{ KW_CACHEPROB, "CacheProb",0},
	{ KW_STOREPROB, "StoreProb",0},
	{ KW_NGBRSTORPROB, "NeighborStoreProb",0},
	{ KW_CACHESIZE, "CacheSize",0},
	{ KW_PERMSIZE, "PermSize",0 },
	{ KW_RETRY, "Retry",0 },
	{ INVALID, NULL, 0 }
};
/*
This method will produce UOID using SHA key generation.
*/
char *GetUOID(char *node_inst_id, char *obj_type, char *uoid_buf, int uoid_buf_sz){
	static unsigned long seq_no=(unsigned long)1;
    char sha1_buf[SHA_DIGEST_LENGTH], str_buf[104];

    snprintf(str_buf, sizeof(str_buf), "%s_%s_%1ld", node_inst_id, obj_type, (long)seq_no++);
    SHA1((unsigned char*)str_buf, strlen(str_buf), (unsigned char*)sha1_buf);
    memset(uoid_buf, 0, uoid_buf_sz);
    memcpy(uoid_buf, sha1_buf, min(uoid_buf_sz,sizeof(sha1_buf)));
    return uoid_buf;
}
/*
ebugging routine
*/
void printhex(unsigned char *src ,int n){
	char *toprint = (char*)malloc(n*2 + 1);
	toprint[n*2] = '\0';
	int i = 0;
	for (; i<n; i++){
		sprintf(toprint+i*2, "%02X", src[i]);
	}
//	printf("length %d\n", n);
	//printf("%s\n", toprint);
	free(toprint);
}
/*
binary to hex conversion routine
*/
void int2hex(unsigned char *src ,int n, char* toprint){	
	toprint[n*2] = '\0';
	int i = 0;
	for (; i<n; i++){
		sprintf(toprint+i*2, "%02X", src[i]);
	}
}

/*this method will remove the leading and trailing whitespace from the string
*/
int TrimBlanks(char *str){
    size_t len = 0;
    char *front = str;
    char *end = NULL;
	//if string is null or empty return
    if( str != NULL ||  str[0] != '\0'){
		len = strlen(str);
		end = str + len - 1 ;
		//remove starting whitespace
		while( isspace(*front)){
			front++;
		}
		//remove trailing whitespace
		while( isspace(*end) ){
			end--;
			if(end == front){
				break;
			}
		}
		//put '\0' at the end of the string
		if( str + len - 1 != end ){
			*(end + 1) = '\0';
		}
		else if( front != str &&  end == front ){
			*str = '\0';
		}
		end = str;
		//copy the string at the satrt location, so that string will start from the start location
		if( front != str ){
			while( *front ) {
				*end++ = *front++;
			}
			*end = '\0';
		}
		return 0;
	}
	return -1;
}
/*
get key value pair from the ini file
*/
int GetKeyValue(char *buf, char separator, char **ppsz_key, char **ppsz_value){
	char *psz_value=strchr(buf, separator);
	if (psz_value == NULL) {
		return -1;
	}
	*psz_value++ = '\0';
	TrimBlanks(buf);
	TrimBlanks(psz_value);
	if (ppsz_key != NULL) *ppsz_key = buf;
	if (ppsz_value != NULL) *ppsz_value = psz_value;

	return 0;
}
/*write bytes into the socket
*/
int writeByte(int fd, unsigned char* vptr, int n){
	int		nleft;
	int		nwritten;
	unsigned char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (errno == EINTR){
				nwritten = 0;					
			}
			else{				
				fprintf(logfile, "** Write error\n");
				fflush(logfile);
				return(-1);			
			}
		}
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
/*read bytes from the socket
*/
int readByte(int fd, unsigned char* vptr, int n ){
	int	nleft;
	int	nread;
	unsigned char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR){
				nread = 0;		
			}
			else{								
				return(-1);
			}
		} else if (nread == 0){
			break;				
		}

		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);		
}

//write one byte at a time into the socket
/*int writeByte(int fd, unsigned char* vptr, int n ){
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
int readByte(int fd, unsigned char* vptr, int n ){
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
}*/
/*
This will generate the instance id for the node.
*/
void getNodeInstId(char* id_buf, char* instName){
	strcpy(id_buf, instName);
	int len = strlen(id_buf); 
	id_buf[len] = '_';
	len++;
	sprintf(id_buf + len, "%ld", (long)starttime);
}
/*
This method produce the node id given the connection information of the node.
*/
void getNodeId(char* name_buf, char* hostname, uint16_t portno){
	strcpy(name_buf, hostname);
	int len = strlen(name_buf); 
	name_buf[len] = '_';
	len++;
	sprintf(name_buf + len, "%d", portno);
	len = strlen(name_buf);
	name_buf[len] = '\0';
}

//signla handler for SIGPIPE
void sigpipe_handler(int sig_num){
	signal(SIGPIPE, SIG_IGN);
}
void sigusr1_handler(int sig_num){
	//isShutdown = 1;	
}
/*This method first checks is the ip address is already in the
dotted format then it return the passed string otherwise
it get the ip by calling gethostbyname
*/

char* getIPAddr(char* host){
	if(host[0] - '0' > 0 && host[0] - '0' < 9){
		//hostname is in dotted decimal format
		return host;
	}
	else{
		struct hostent *he;
		he = gethostbyname(host);
		if (he == NULL) { // do some error checking
		    //gethostbyname
			fprintf(logfile, "** gethostbyname failed\n");fflush(logfile);
		    return NULL;
		}
		return inet_ntoa(*(struct in_addr*)he->h_addr);
	}
}
/*
A generic method for sending all type of packets.
*/
int sendMessage(int sock_id, Message* msg, char* sentid){
	int msg_buf_sz = 27 + msg->dataLength;
	unsigned char *msg_buf = (unsigned char*)malloc(msg_buf_sz);
	if (msg_buf == NULL) {
		fprintf(logfile, "** malloc() failed\n");
		fflush(logfile);
	}
	memset(msg_buf, 0, msg_buf_sz);
	int off = 0;
	memcpy(msg_buf + off, &(msg->type), 1); off += 1;
	memcpy(msg_buf + off, msg->uoid, 20); off += 20;
	memcpy(msg_buf + off, &(msg->ttl), 1); off += 1;
	off += 1; //reserved
	memcpy(msg_buf + off, &(msg->dataLength), 4); off += 4;
	if(msg->dataLength > 0){
		memcpy(msg_buf + off, msg->data, msg->dataLength); off += msg->dataLength;
	}
	if(writeByte(sock_id, msg_buf, msg_buf_sz) != msg_buf_sz) {
		fprintf(logfile, "** Write error\n");
		fflush(logfile);
		free(msg_buf);
		return -1;
	}
///////////////log the message//////////////////////
	if(msg->type == HELLO){
		printlogHello(msg, msg->data, sentid, 1);
	}
	else if(msg->type == KEEPALIVE){
		writeLog(SEND, sentid, msg->dataLength, msg->ttl, KPAV, "", msg->uoid);
	}
	else if(msg->type == NOTIFY){
		writeLog(SEND, sentid, msg->dataLength, msg->ttl, NTFY, "1", msg->uoid); //for now hardcoded
	}	
///////////////////////////////////////////////////
	if(msg->dataLength > 0){
		free(msg->data);
	}
	free(msg);
	free(msg_buf);
	return 0;
}
/*This method search for the router entry in the routing table and
return NULL if not found else return the routing entry
*/
RouterNode* findRouterEntry(int8_t* uoid){
	ListElement* head = routerhead;
	while(head != NULL){					
		RouterNode* route = (RouterNode*)head->item;		
		if((route->lifetime > 0) && !strncmp(route->uoidkey, uoid, 20)){
			return route;
		}
		head = head->next;
	}
	return NULL;	
}

void reduceLifeTime(){
	ListElement* head = routerhead;
	//ListElement* pre = NULL;
	while(head != NULL){					
		RouterNode* route = (RouterNode*)head->item;		
		//printf("route %s\n" , route->tosend->nodeId);
		if(route->lifetime > 0){
			route->lifetime--;
		}
		/*if(route->lifetime == 0){
			if(pre == NULL){
				routerhead = head->next;
				free(head);
				head = routerhead;
			}
			else{
				pre->next = head->next;
				free(head);
				head = pre->next;
			}
			continue;
		}
		pre = head;*/
		head = head->next;
	}
}
/*
method for writing log in the log file
*/
void writeLog(int dir, char* nodeid, int size, int ttl, int type, char* data, int8_t* msguoid){	
	struct timezone tz;
	struct timeval tv;
	gettimeofday(&tv, &tz);	
	char time[256];

	char toprint[9];
	int2hex((unsigned char*)(msguoid + 16), 4, toprint);			

	if(dir == 3 || dir == 4){ 
		fprintf(logfile, "%s %s\n",  msgdir[dir], data);
	}
	else{
		memset(time, 0, 256);
		snprintf(time, 256, "%10ld.%03d", ((unsigned long int)tv.tv_sec*1000 + tv.tv_usec/1000), (int)tv.tv_usec%1000);		
		pthread_mutex_lock (&printLock);
		fprintf(logfile, "%s %s %s %s %d %d %s %s\n",  msgdir[dir], time, nodeid, msgtype[type], size+27, ttl, toprint, data);
	}
	fflush(logfile);
	pthread_mutex_unlock (&printLock);
}
/*
This method forward the same message after reducing its ttl by one 
to the the connected node as per the routing table entry.
*/
void writingSameResMsg( Message* msg, unsigned char* data, Node* ngbrtosend){
	Message * response = (Message*)malloc(sizeof(Message));
	if (response == NULL) {
		fprintf(logfile, "** malloc() failed\n");
		fflush(logfile);
	}				
	memcpy(response, msg, sizeof(Message));
	
	response->data = (unsigned char*)malloc(msg->dataLength);
	memcpy(response->data, data, msg->dataLength);
	if(response->type == STATUSRESPONSE){
		char toprint[9];
		int2hex((unsigned char*)(response->data + 16), 4, toprint);
		writeLog(FORW, ngbrtosend->nodeId, response->dataLength, response->ttl, STRS, toprint, response->uoid);
	}
	else if(response->type == JOINRESPONSE){
		printJoinResponse(response,  response->data, ngbrtosend->nodeId, 2);
	}
	else if(msg->type == CHECKRESPONSE){
		char toprint[9];
		int2hex((unsigned char*)(response->data + 16), 4, toprint);		
		writeLog(FORW, ngbrtosend->nodeId, response->dataLength, response->ttl, CKRS, toprint, response->uoid);
	}
	pthread_mutex_lock(&(ngbrtosend->nodeLock));				
	Append(&(ngbrtosend->writeQhead), &(ngbrtosend->writeQtail), response);
	ngbrtosend->qlen++;
	pthread_mutex_unlock(&(ngbrtosend->nodeLock));
	pthread_cond_signal(&(ngbrtosend->nodeCond));							
}
/*
This method forward the same response message to the the connected node as per the routing table entry.
*/
void writingSameResMsgttldec( Message* msg, unsigned char* data, Node* ngbrtosend){
	Message * response = (Message*)malloc(sizeof(Message));
	if (response == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}				
	memcpy(response, msg, sizeof(Message));
	response->ttl--;
	response->data = (unsigned char*)malloc(msg->dataLength);
	memcpy(response->data, data, msg->dataLength);
	
	if(msg->type == STATUS){
		//if(msg->data[0] == 1){
			writeLog(FORW, ngbrtosend->nodeId, response->dataLength, response->ttl, STRQ, "neighbors", response->uoid);
		//}
		//else{
		//	writeLog(FORW, ngbrtosend->nodeId, response->dataLength, response->ttl, STRQ, "files", response->uoid);
		//}
	}
	else if(msg->type == JOIN){		
		printJoin(response, response->data, ngbrtosend->nodeId, 2);
	}
	else if(msg->type == CHECK){
		writeLog(FORW, ngbrtosend->nodeId, response->dataLength, response->ttl, CKRQ, "", response->uoid);
	}

	pthread_mutex_lock(&(ngbrtosend->nodeLock));				
	Append(&(ngbrtosend->writeQhead), &(ngbrtosend->writeQtail), response);
	ngbrtosend->qlen++;
	pthread_mutex_unlock(&(ngbrtosend->nodeLock));
	pthread_cond_signal(&(ngbrtosend->nodeCond));							
}
/*
parser routine for parsing ini file.
*/
int iniparser(char * filename){
	FILE* file = fopen (filename, "r");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file %s open fails\n", filename);fflush(logfile);
		return -1;
	}
	char line[MAX_LINE_SIZE];
	memset(line, 0, MAX_LINE_SIZE);
	char *psz_key=NULL, *psz_value=NULL;

	while(NULL != fgets(line, MAX_LINE_SIZE, file)){		
		if (GetKeyValue(line, '=', &psz_key, &psz_value) == 0) {			
			if(strlen(psz_value) == 0){					
				char *temp = strchr(psz_key, ':');
				if(temp != NULL){
					*temp = '\0'; temp++;															
					uint16_t portno = atoi(temp);	
					if(!strcmp(myhostname, psz_key) && (portno == myport)){
						isBeacon = 1;						
					}
					else{										
						Node* bnode = createNode(psz_key, portno);
						Append(&beaconshead, &beaconstail, bnode);					
					}
				}
			}
			else{
				//KwInfo *pwi = gkwinfo;
				int found_id=(-1);
				int i;
				for (i = 0; gkwinfo[i].key != NULL; i++) {
					//if (pwi->key == NULL) break;
					if (strncasecmp(gkwinfo[i].key, psz_key, strlen(gkwinfo[i].key)) == 0) {
						found_id = gkwinfo[i].id;						
						break;
					}
				}
				if (found_id == (-1)) {
					//print a warning message					
				}
				else{
					processvalue(found_id, psz_value);		
				}
			}
		}
		memset(line, 0, MAX_LINE_SIZE);
	}	
	fclose(file);

	getNodeId(mynodeId, myhostname, myport);
	memset(logfilePath, 0, 512);
	if(logfileName[0] == '/'){
		strcpy(logfilePath, logfileName);
	}
	else{						
		sprintf(logfilePath, "%s/%s", homeDir, logfileName);
	}
	memset(initNgbrPath, 0, 512);
	sprintf(initNgbrPath, "%s/init_neighbor_list", homeDir);

	return 0;
}
/*
parser routine for parsing init_neighbors_list file.
*/
int ngbrParser(){ 	
	FILE* file = fopen(initNgbrPath, "r");
	char line[MAX_LINE_SIZE];
	memset(line, 0, MAX_LINE_SIZE);
	
	while(NULL != fgets(line, MAX_LINE_SIZE, file)){		
		TrimBlanks(line);
		if(line[0] == '\0'){
			continue;
		}
		char *temp = strchr(line, ':');
		if(temp != NULL){
			*temp = '\0'; temp++;
			uint16_t portno = atoi(temp);	

			Node* ngbrnode = createNode(line, portno);
			Append(&negbhead, &negbtail, ngbrnode);		
		}
		else {
			fclose(file);
			return -1;
		}
	}
	fclose(file);
	return 0;
}
void processvalue(int id, char * value){
	switch (id) {
		case KW_PORT: 
			myport = atoi(value); 
		break;
		case KW_LOCATION: 
			location = atol(value); 
		break;
		case KW_HOMEDIR: 
			memset(homeDir, 0, 512);
			strcpy(homeDir, value);
			mkdir(homeDir, S_IRWXU | S_IRWXG | S_IRWXO);
		break;
		case KW_LOGFILE:
			memset(logfileName, 0, 512);
			strcpy(logfileName, value);
		break;
		case KW_AUTOSHUTDOWN:
			autoShutdown = atoi(value); 
		break;
		case KW_TTL:
			ttl = atol(value); 
		break;
		case KW_MSGLIFETIME:
			msgLifeTime = atoi(value); 
		break;
		case KW_GETMSGLIFETIME:
			getMsgLifeTime = atoi(value); 
		break;
		case KW_INITNGBR:
			initNgbr = atoi(value); 
		break;
		case KW_JOINTIMEOUT:
			joinTimeout = atoi(value); 
		break;
		case KW_KEEPALIVE:
			keepalive = atoi(value); 
		break;
		case KW_MINNGBR:
			minNgbr = atoi(value); 
		break;
		case KW_NOCHECK:
			noCheck = atoi(value); 
		break;
		case KW_CACHEPROB:
			cacheProb = strtod(value, NULL);  
		break;
		case KW_STOREPROB:
			storeProb = strtod(value, NULL);  
		break;
		case KW_NGBRSTORPROB:
			ngbrStoreProb = strtod(value, NULL); 
		break;
		case KW_CACHESIZE:
			cacheSize = atoi(value); 
		break;
		case KW_RETRY:
			retry = atoi(value); 
		break;
		case KW_PERMSIZE:
			permSize = atoi(value); 
		break;
	}
}
/*
This method read the data into message structure
*/
void readMessage(Message* msg, unsigned char* data){
	memset(msg, 0, sizeof(Message));	
	int off = 0;
	memcpy(&(msg->type), data + off,  1); off += 1;
	memcpy(msg->uoid, data + off, 20); off += 20;
	memcpy(&(msg->ttl), data + off, 1); off += 1;
	off += 1; //reserved
	memcpy(&(msg->dataLength), data + off, 4); off += 4;				
}

/*
Method for creatinging a node with the connection information.
*/
Node* createNode(char* hostname, uint16_t portno){
	Node* ngbrnode = (Node*)malloc(sizeof(Node));
	if(ngbrnode == NULL){
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}
	ngbrnode->port = portno;
	memset(ngbrnode->hostname, 0, 512);
	strcpy(ngbrnode->hostname, hostname);
	memset(ngbrnode->nodeId, 0, 512);
	getNodeId(ngbrnode->nodeId, ngbrnode->hostname, ngbrnode->port);
	
	ngbrnode->writeQhead = NULL;
	ngbrnode->writeQtail = NULL;						
	ngbrnode->isHelloDone = 0;
	ngbrnode->terminate = 0;
	ngbrnode->kplvLifeTime = keepalive;
	pthread_mutex_init(&(ngbrnode->nodeLock), NULL);
	pthread_cond_init(&(ngbrnode->nodeCond), NULL);
	ngbrnode->qlen = 0;
	ngbrnode->istemp = 0;

	return ngbrnode;
}

/*
Method for creatinging a node without the connection information.
*/
Node* createEmptyNode(){
	Node* ngbrnode = (Node*)malloc(sizeof(Node));
	if(ngbrnode == NULL){
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}
	/*ngbrnode->port = portno;
	memset(ngbrnode->hostname, 0, 512);
	strcpy(ngbrnode->hostname, hostname);
	memset(ngbrnode->nodeId, 0, 512);
	getNodeId(ngbrnode->nodeId, ngbrnode->hostname, ngbrnode->port);*/
	
	ngbrnode->writeQhead = NULL;
	ngbrnode->writeQtail = NULL;						
	ngbrnode->isHelloDone = 0;
	ngbrnode->terminate = 0;
	ngbrnode->kplvLifeTime = keepalive;
	pthread_mutex_init(&(ngbrnode->nodeLock), NULL);
	pthread_cond_init(&(ngbrnode->nodeCond), NULL);
	ngbrnode->qlen = 0;
	ngbrnode->istemp = 0;
	return ngbrnode;
}
/*
Method for destroying a node.
*/
void destroyNode(Node* n){
	//destroy all the items in the queue
	ListElement* head = n->writeQhead;
	while(head != NULL){					
		Message* msg = (Message*)head->item;				
		free(msg->data);
		free(msg);
		head = head->next;
	}
	pthread_mutex_destroy(&(n->nodeLock));
	pthread_cond_destroy(&(n->nodeCond));
	destroyList(&(n->writeQhead), &(n->writeQtail));	
	free(n);
}
/*
This will put a new routing entry into the routing table.
*/
void putNewRouteEntry(int8_t* uoid, Node* ngb){
   //put the new uoid into the routing table
	RouterNode *rnode = (RouterNode*)malloc(sizeof(RouterNode));				
	if(rnode == NULL){
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}								
	rnode->lifetime = msgLifeTime;				
	memset(rnode->uoidkey, 0, 20);	
	memcpy(rnode->uoidkey, uoid, 20);				
	rnode->tosend = ngb;				
	pthread_mutex_lock(&routerLock);
	Append(&routerhead, &routerhead, rnode);																						
	pthread_mutex_unlock(&routerLock);	
}


/*
Method for printing log of join message.
*/
void printJoin(Message *msg, unsigned char* data, char* nodeid, int issend){
	char host[512];
	uint16_t  port;
	int off = 4; //for location
	memcpy(&port, data + off, 2); off += 2;
	memset(host, 0, 512);
	memcpy(host, data + off, msg->dataLength - off); 
	host[msg->dataLength - off] = '\0';
	char temp[512];
	memset(temp, 0, 512);
	snprintf(temp, 512, "%d %s",  port, host);
	if(issend == 2){
		writeLog(FORW, nodeid, msg->dataLength, msg->ttl, JNRQ, temp, msg->uoid);
	}
	else if(issend == 1){
		writeLog(SEND, nodeid, msg->dataLength, msg->ttl, JNRQ, temp, msg->uoid);
	}
	else{
		writeLog(RCV, nodeid, msg->dataLength, msg->ttl, JNRQ, temp, msg->uoid);
	}
}

/*
Method for printing log of join response message.
*/
void printJoinResponse(Message *msg, unsigned char* data, char* nodeid, int issend){
	int8_t uoid[20];
	char host[512];
	uint16_t port;
	uint32_t distance;
	int off = 0;
	memset(uoid, 0, 20);
	memcpy(uoid, data + off, 20); off += 20;							
	memcpy(&(distance), data + off, 4); off += 4;
	memcpy(&(port), data + off, 2); off += 2;
	memset(host,0,512);
	memcpy(host, data + off,  msg->dataLength - off);  
	host[msg->dataLength - off] = '\0';
	
	char toprint[9];
	int2hex((unsigned char*)(uoid + 16), 4, toprint);
	char temp[512];		
	memset(temp, 0, 512);
	sprintf(temp, "%s %d %d %s", toprint, distance, port, host);
	if(issend == 2){
		writeLog(FORW, nodeid, msg->dataLength, msg->ttl, JNRS, temp, msg->uoid);	
	}
	else if(issend == 1){
		writeLog(SEND, nodeid, msg->dataLength, msg->ttl, JNRS, temp, msg->uoid);	
	}
	else{
		writeLog(RCV, nodeid, msg->dataLength, msg->ttl, JNRS, temp, msg->uoid);	
	}
}
/*
Method for printing log of hello message.
*/
void printlogHello(Message* msg, unsigned char* data, char* nodeid, int issend){
	uint16_t otherport;
	char otherhost[512];
	memset(otherhost,0,512);
	int off = 0;
	memcpy(&otherport, data + off, 2); off += 2;		
	memcpy(otherhost, data + off, msg->dataLength - 2);

	otherhost[msg->dataLength - 2] = '\0';
	
	char temp[512];
	memset(temp, 0, 512);
	snprintf(temp, 512, "%d %s",  otherport , otherhost);		
	
	if(issend == 1){
		writeLog(SEND, nodeid, msg->dataLength, msg->ttl, HLLO, temp, msg->uoid);
	}
	else{
		writeLog(RCV, nodeid, msg->dataLength, msg->ttl, HLLO, temp, msg->uoid);
	}
}
/*
Search for the node having the same nodeId, used by hello 
methods to resolve the synchronous connection problem.
*/
Node* findNode(Node* nei, ListElement* head){
	ListElement* temphead = head;
	while(temphead != NULL){					
		Node* ngb = (Node*)temphead->item;		
		if((ngb != nei) && !strcmp(ngb->nodeId, nei->nodeId)){
			return ngb;
		}
		temphead = temphead->next;
	}
	return NULL;	
}

/*signla handler for SIGALRM
*/
void sigalrm_handler(int sig_num){
	isShutdown = 1;
	pthread_kill(inputThread, SIGUSR1);	
}
