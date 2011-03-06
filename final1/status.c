#include "status.h"
///////////////////////////////////////////
/*
Flags and data structure for status message type.
*/
int isStatusOn = 0;
int8_t statusuoid[20];
pthread_t statusThread;
StatusNode statusDB[10];
int statusDBlen = 0;
pthread_mutex_t statusDBLock;
typedef struct id2node{
	int id;
	char nodeid[512];
}Id2Node;

///////////////////////////////////////////
/*
A no-op interrupt handling routine.
*/
void interrupt(int sig){
	
}
/*
Status handler thread routine. It will sleeps for msgLifeTime seconds
and after that collect the network topology information and put them 
into nam format into a status file.
*/
void* statusHandler(void *dummy){	
	sigset_t newsig;
	sigemptyset(&newsig);
	sigaddset(&newsig, SIGINT);
	sigaddset(&newsig, SIGTERM);

	struct sigaction act;
	act.sa_handler = interrupt;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	pthread_sigmask(SIG_UNBLOCK, &newsig, NULL);

	struct timeval delay;
	delay.tv_sec = msgLifeTime;
	delay.tv_usec = 0;
	select(0, NULL, NULL, NULL, &delay);
	isStatusOn = 0;
	//////////////////
	pthread_mutex_lock(&(statusDBLock));				
	int index = statusDBlen++;
	memset(statusDB[index].nodeId, 0, 512);
	strcpy(statusDB[index].nodeId, mynodeId);
	int inei = 0;
	ListElement* head = conhead;			
	while(head != NULL){			
		Node* nei = (Node*)head->item;
		if(nei->isHelloDone == 1){				
			strcpy(statusDB[index].neibr[inei], nei->nodeId); 
			inei++;
		}
		head = head->next;
	}		
	statusDB[index].nnei = inei;
	//////////////////////
	//print the packets
	int i, j;
	Id2Node map[10];
	int maplen = 0;
	FILE* file = fopen (statusFilename, "w");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file init_neighbor_list open fails\n");fflush(logfile);
	}							
	fprintf(file, "V -t * -v 1.0a5\n");	
	for(i = 0 ;i < statusDBlen; ++i){
		map[maplen].id = i + 1;
		memset(map[maplen].nodeid, 0, 512);
		strcpy(map[maplen].nodeid, statusDB[i].nodeId);		
		fprintf(file, "n -t * -s %d -c red -i black\n",  map[maplen].id);
		maplen++;
	}	
	for(i = 0 ;i < statusDBlen; ++i){
		int x = i + 1;
		int y;
		for(j = 0; j < statusDB[i].nnei; ++j){
			int k;
			for(k = 0; k < maplen; ++k){				
				if(!strcmp(statusDB[i].neibr[j], map[k].nodeid)){
					y = map[k].id;
					break;
				}
			}
			if(k == maplen){
				map[maplen].id = maplen + 1;
				memset(map[maplen].nodeid, 0, 512);
				strcpy(map[maplen].nodeid, statusDB[i].neibr[j]);
				y = map[maplen].id;
				fprintf(file, "n -t * -s %d -c red -i black\n",  map[maplen].id);
				maplen++;				
			}
			fprintf(file, "l -t * -s %d -d %d -c blue\n",  x, y);
		}
	}	
	fflush(file);
	fclose(file);
	pthread_mutex_unlock(&(statusDBLock));				

	/*for(i = 0 ;i < statusDBlen; ++i){
		printf("node %s has %d neighbors\n", statusDB[i].nodeId, statusDB[i].nnei);
		for(j = 0; j < statusDB[i].nnei; ++j){
			printf("%s ,", 	statusDB[i].neibr[j]);
			fflush(stdout);
		}
	}*/

	pthread_exit(NULL);
}
/*
This method will build the status message to send.
*/
void prepareStatusWithUoid(Message *msg, uint8_t ttl, int isNgbr, int8_t* uoid){
	memset(msg, 0, sizeof(Message));
	msg->type = STATUS;
	memset(msg->uoid, 0, 20);
	memcpy(msg->uoid, uoid, 20);
	msg->ttl = ttl;	
	msg->dataLength = 1;	
	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	uint8_t type;
	if(isNgbr){
		type = STATUS_TYPE_NEIGHBORS;
	}
	else{
		type = STATUS_TYPE_FILES;
	}
	memcpy(msg->data, &(type), msg->dataLength); 
}
/*
This methos prepare the response of the status message and flood thme
to the network.
*/
void sendStatusResponse(int isNgbr, uint32_t length, int8_t* uoid, Node* ngb){
	Message * msg = (Message*)malloc(sizeof(Message));
	if (msg == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}
	memset(msg, 0, sizeof(Message));
	msg->type = STATUSRESPONSE;
	char inst_id[512];	
	memset(inst_id, 0, 512);	
	memset(msg->uoid, 0, 20);
	getNodeInstId(inst_id, mynodeId);
	GetUOID(inst_id, "msg", msg->uoid, 20);
	msg->ttl = 1;	
	msg->dataLength = length;	

	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	int off = 0;
	memcpy(msg->data + off, uoid, 20); off += 20;
	//if(isNgbr){		
		uint16_t recordlen = 2 + strlen(myhostname);
		memcpy(msg->data + off, &(recordlen), 2); off += 2;
		memcpy(msg->data + off, &(myport), 2); off += 2;		
		memcpy(msg->data + off, myhostname, strlen(myhostname)); off += strlen(myhostname);

		ListElement* head = conhead;			
		while(head != NULL){			
			Node* nei = (Node*)head->item;
			if(nei->isHelloDone == 1){	
				recordlen = 2 + strlen(nei->hostname);						
				memcpy(msg->data + off, &(recordlen), 2); off += 2;	
				memcpy(msg->data + off, &(nei->port), 2); off += 2;
				memcpy(msg->data + off, nei->hostname, strlen(nei->hostname)); off += strlen(nei->hostname);				
			}
			head = head->next;
		}		
	//}
	//else{
		//files
	//}			
	char toprint[9];
	int2hex((unsigned char*)(uoid + 16), 4, toprint);				
	writeLog(SEND, ngb->nodeId, msg->dataLength, msg->ttl, STRS, toprint, msg->uoid);	
	pthread_mutex_lock(&(ngb->nodeLock));				
	Append(&(ngb->writeQhead), &(ngb->writeQtail), msg);
	ngb->qlen++;
	pthread_cond_signal(&(ngb->nodeCond));
	pthread_mutex_unlock(&(ngb->nodeLock));
}
/*
This method will extract the network topology information from the
status response it has got.
*/
void storeStatusInfoNgbr(int len, unsigned char* data){
	char otherhost[512];
	uint16_t otherport;
	uint16_t reclen;
	int off = 20; //for uoid
	pthread_mutex_lock(&(statusDBLock));				
	int index = statusDBlen++;
	pthread_mutex_unlock(&(statusDBLock));				
	memcpy(&reclen, data + off, 2); off += 2;
	memcpy(&otherport, data + off, 2); off += 2;
	memset(otherhost,0,512);
	memcpy(otherhost, data + off,  reclen - 2); off += reclen - 2;
	otherhost[reclen - 2] = '\0';
	memset(statusDB[index].nodeId, 0, 512);
	getNodeId(statusDB[index].nodeId, otherhost, otherport );
	int inei = 0;
	while(off != len){
		memcpy(&reclen, data + off, 2); off += 2;
		memcpy(&otherport, data + off, 2); off += 2;
		memset(otherhost, 0,512);
		memcpy(otherhost, data + off,  reclen - 2); off += reclen - 2;
		otherhost[reclen - 2] = '\0';
		memset(statusDB[index].neibr[inei], 0, 512);
		getNodeId(statusDB[index].neibr[inei], otherhost, otherport );					
		inei++;
	}
	statusDB[index].nnei = inei;
}
