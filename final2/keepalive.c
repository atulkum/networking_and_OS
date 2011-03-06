#include "keepalive.h"

pthread_t keepaliveThread;
/*
This method would build the KEEPALIVE message.
*/
void prepareKeepalive(Message *msg){
	memset(msg, 0, sizeof(Message));
	char inst_id[512];	
	memset(inst_id, 0, 512);	
	msg->type = KEEPALIVE;
	memset(msg->uoid, 0, 20);
	getNodeInstId(inst_id, mynodeId);
	GetUOID(inst_id, "msg", msg->uoid, 20);
	msg->ttl = 1;	
	msg->dataLength = 0;
	msg->data = NULL;
}
/*
keepalive handler thread routine. It will send keep alive message to 
every connected neighbor at every keepalive/2 time interval.
*/
void* keepaliveHandler(void *dummy){	
	for(;;){		
		if(isShutdown){
			break;
		}
		struct timeval delay;
		delay.tv_sec = keepalive/2;
		delay.tv_usec = 0;
		select(0, NULL, NULL, NULL, &delay);		

		if(isShutdown){
			break;
		}
		ListElement* head = conhead;
		while(head != NULL){					
			Node* nei = (Node*)head->item;
			if(nei->isHelloDone == 1){
				Message * keepalive = (Message*)malloc(sizeof(Message));
				if (keepalive == NULL) {
					fprintf(logfile, "** malloc() failed\n");fflush(logfile);
				}
				prepareKeepalive(keepalive);
				pthread_mutex_lock(&(nei->nodeLock));				
				Append(&(nei->writeQhead), &(nei->writeQtail), keepalive);
				nei->qlen++;
				pthread_mutex_unlock(&(nei->nodeLock));
				pthread_cond_signal(&(nei->nodeCond));	
			}
			head = head->next;
		}		
	}
	pthread_exit(NULL);
}

