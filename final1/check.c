#include "check.h"

pthread_t checkThread;
int isCheckOk = 0;
int8_t checkuoid[20];
/*
This method would build the CHECK message.
*/
void prepareCheck(Message *msg, int8_t* uoid){
	memset(msg, 0, sizeof(Message));
	msg->type = CHECK;
	memset(msg->uoid, 0, 20);
	memcpy(msg->uoid, uoid, 20);
	msg->ttl = ttl;	
	msg->dataLength = 0;
	msg->data = NULL;
}
/*
This method prepare the response of the check message and flood thme
to the network.
*/
void prepareCheckResponse(Message *msg, int8_t* uoid){
	memset(msg, 0, sizeof(Message));
	char inst_id[512];	
	memset(inst_id, 0, 512);	
	msg->type = CHECKRESPONSE;
	memset(msg->uoid, 0, 20);
	getNodeInstId(inst_id, mynodeId);
	GetUOID(inst_id, "msg", msg->uoid, 20);
	msg->ttl = 1;	
	msg->dataLength = 20;
	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	memcpy(msg->data, uoid, 20); 
}
/*
This method sleeps for joinTimeout second and after waking up
see if there is any reponse it has got or not. It no reponse is there
it will do soft restart.
*/
void* checkHandler(void *dummy){			
	struct timeval delay;
	delay.tv_sec = joinTimeout;
	delay.tv_usec = 0;
	select(0, NULL, NULL, NULL, &delay);
	if(!isCheckOk){
		//initiated soft restart
		//after deleting init ngbr list
		//remove(initNgbrPath);
		fprintf(logfile, "** Does not get proper check response.Restarting\n"); fflush(logfile);
		softshutdown = 1;					
		pthread_kill(inputThread, SIGUSR1);	
	}
	pthread_exit(NULL);
}
/*
This method intiate the check flooding and send the check message to
every of its connecting neighbors.
*/
void doCheck(){
	if(!isBeacon && !noCheck){		
		isCheckOk = 0;
		char inst_id[512];	
		int8_t uoid[20];
		memset(inst_id, 0, 512);	
		memset(uoid, 0, 20);
		getNodeInstId(inst_id, mynodeId);
		GetUOID(inst_id, "msg", uoid, 20);
		memset(checkuoid, 0, 20);
		memcpy(checkuoid, uoid, 20);

		ListElement* head = conhead;
		while(head != NULL){					
			Node* nei = (Node*)head->item;
			if(nei->isHelloDone == 1){
				Message* check = (Message*)malloc(sizeof(Message));
				if (check == NULL) {
					fprintf(logfile, "** malloc() failed\n");
					fflush(logfile);
				}
				prepareCheck(check, uoid);						
				writeLog(SEND, nei->nodeId, check->dataLength, check->ttl, CKRQ, "", uoid);					
				pthread_mutex_lock(&(nei->nodeLock));				
				Append(&(nei->writeQhead), &(nei->writeQtail), check);
				nei->qlen++;
				pthread_mutex_unlock(&(nei->nodeLock));
				pthread_cond_signal(&(nei->nodeCond));			
			}
			head = head->next;
		}				
		pthread_create(&(checkThread), NULL, checkHandler, (void*)NULL);
	}	
}
/*
This method will send the check response in case of the node is a beacon node.
*/
void sendCheckResponse(int8_t* uoid, Node* ngb){
	Message * msg = (Message*)malloc(sizeof(Message));
	if (msg == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}
	memset(msg, 0, sizeof(Message));
	msg->type = CHECKRESPONSE;
	char inst_id[512];	
	memset(inst_id, 0, 512);	
	memset(msg->uoid, 0, 20);
	getNodeInstId(inst_id, mynodeId);
	GetUOID(inst_id, "msg", msg->uoid, 20);
	msg->ttl = 1;	
	msg->dataLength = 20;	

	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	int off = 0;
	memcpy(msg->data + off, uoid, 20); off += 20;

	char toprint[9];
	int2hex((unsigned char*)(uoid + 16), 4, toprint);			
	
	writeLog(SEND, ngb->nodeId, msg->dataLength, msg->ttl, CKRS, toprint, msg->uoid);
	
	pthread_mutex_lock(&(ngb->nodeLock));				
	Append(&(ngb->writeQhead), &(ngb->writeQtail), msg);
	ngb->qlen++;
	pthread_cond_signal(&(ngb->nodeCond));
	pthread_mutex_unlock(&(ngb->nodeLock));
}

