#include "join.h"
///////////////////////////////////////////
/*
Flags and data structure for join message type.
*/
pthread_t joinThread;
JoinNode joinDB[10];
int joinDBlen = 0;
pthread_mutex_t joinDBLock;
int isJoinDone = 0;
int joinSuccessfull = 0;
int8_t joinuoid[20];
///////////////////////////////////////////
/*
Join handler thread routine. It will sleeps for joinTimeout seconds
and after that sort the collected information about the distance of nodes.
And put the least distance neibors in  init_neighbors_list file.
*/
void* joinHandler(void *dummy){
	int sock_id = (int)dummy;
	struct timeval delay;
	delay.tv_sec = joinTimeout;
	delay.tv_usec = 0;
	select(0, NULL, NULL, NULL, &delay);	
		
	shutdown(sock_id, SHUT_RDWR);
	close(sock_id);	
	//int i;
	//for(i = 0 ;i < joinDBlen; ++i){
	//	printf("node %s:%d has %d distance\n", joinDB[i].host, joinDB[i].port, joinDB[i].distance);
	//}
	if(initNgbr >  joinDBlen){
		printf("\nNot Enough Join Responses. Quiting!\n");
		joinSuccessfull = 0;
		pthread_exit(NULL);
	}
	else{
		joinSuccessfull = 1;
	}
	if(joinDBlen > 0){			
		unsigned char sorted[10];
		uint32_t mindist;
		int minid;
		int isfirst;
		int j = 0;
		int i;
		for(i = 0 ;i < joinDBlen; ++i){
			joinDB[i].isdone = 0;
		}
		for(j = 0 ; j < initNgbr; ++j){
			isfirst = 1;
			for(i = 0 ; i < joinDBlen; ++i){
				if(!joinDB[i].isdone){
					if(isfirst){
						mindist = joinDB[i].distance;
						minid = i;
						isfirst = 0;
					}
					else{
						if(mindist > joinDB[i].distance){
							mindist = joinDB[i].distance;
							minid = i;
						}					
					}
				}				
			}
			sorted[j] = minid;
			joinDB[minid].isdone = 1;
		}	
		
		//write into the file
		FILE* file = fopen (initNgbrPath, "w+");
		if(file == NULL){
			fprintf(logfile, "** ERROR: file init_neighbor_list open fails\n");		
		}						
		for(j = 0 ; j < initNgbr; ++j){		
			fprintf(file, "%s:%d\n", joinDB[sorted[j]].host, joinDB[sorted[j]].port);	
		}
		fclose(file);
	}	
	pthread_exit(NULL);
}
/*
This method would build the JOIN message.
*/
void prepareJoin(Message *msg, int8_t* uoid){
	memset(msg, 0, sizeof(Message));
	msg->type = JOIN;
	memset(msg->uoid, 0, 20);
	memcpy(msg->uoid, uoid, 20);
	msg->ttl = ttl;	
	msg->dataLength = 4 + 2 + strlen(myhostname);		
	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	int off = 0;
	memcpy(msg->data + off, &(location), 4); off += 4;
	memcpy(msg->data + off, &(myport), 2); off += 2;
	memcpy(msg->data + off, myhostname, strlen(myhostname)); 
}
/*
This methos prepare the response of the join message and flood them
to the network.
*/
void prepareJoinResponse(Message *msg, int8_t* uoid, uint32_t rcvloc){
	memset(msg, 0, sizeof(Message));
	msg->type = JOINRESPONSE;
	char inst_id[512];	
	memset(inst_id, 0, 512);	
	memset(msg->uoid, 0, 20);
	getNodeInstId(inst_id, mynodeId);
	GetUOID(inst_id, "msg", msg->uoid, 20);
	msg->ttl = 1;	
	msg->dataLength = 20 + 4 + 2 + strlen(myhostname);	
	
	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	int off = 0;
	memcpy(msg->data + off, uoid, 20); off += 20;
	uint32_t diff;
	if(rcvloc > location){
		diff = rcvloc - location;
	}
	else{
		diff = location - rcvloc;
	}
	//printf("rcvloc %d location %d diff is %d\n", rcvloc, location, diff);
	memcpy(msg->data + off, &(diff), 4); off += 4;
	memcpy(msg->data + off, &(myport), 2); off += 2;
	memcpy(msg->data + off, myhostname, strlen(myhostname)); off += strlen(myhostname); 
}
/*
This method will extract the distance information from the
join response it has got.
*/
void storeJoinInfoNgbr(int len, unsigned char* data){
	int off = 20; //for uoid	
	pthread_mutex_lock(&(joinDBLock));				
	int index = joinDBlen++;
	pthread_mutex_unlock(&(joinDBLock));				

	memcpy(&(joinDB[index].distance), data + off, 4); off += 4;
	memcpy(&(joinDB[index].port), data + off, 2); off += 2;
	memset(joinDB[index].host,0,512);
	memcpy(joinDB[index].host, data + off,  len - 6); 
	joinDB[index].host[len - 6] = '\0';
}
/*
this method will fill up the neighbor connection information from the join
response packet.
*/
void processJoin(Message* msg, unsigned char* temp,  Node* ngbr){	
	char otherhost[512];
	uint16_t otherport;
	int off = 4; //for location
	memcpy(&otherport, temp + off, 2); off += 2;
	memset(otherhost,0,512);
	memcpy(otherhost, temp + off, msg->dataLength - off); 
	otherhost[msg->dataLength - off] = '\0';

	ngbr->port = otherport;
	memset(ngbr->hostname, 0, 512);
	strcpy(ngbr->hostname, otherhost);
	memset(ngbr->nodeId, 0, 512);
	getNodeId(ngbr->nodeId, ngbr->hostname, ngbr->port);

	printJoin(msg, temp, ngbr->nodeId, 0);
}
/*
This method will forward the join message received to every of the connected nighbors
except the node from  where it comes from. It sends a response message to that node
which has sent the join message.
*/
void handleJoinRead(Node* other, Message* msg, unsigned char* data){
	putNewRouteEntry(msg->uoid, other);
	//printf("trap1\n");
	//forward a copy to every neighbor and send a response
	ListElement* head = conhead;			
	while(head != NULL){					
		Node* nei = (Node*)head->item;
		//printf("trap2 %s\n", nei->nodeId);
		//if the node has connection and this is not the node from where 
		//the message came then forward it
		if(nei->isHelloDone == 1 && strcmp(nei->nodeId, other->nodeId)){
			//printf("forward join to %s\n", nei->nodeId);			
			writingSameResMsgttldec(msg, data, nei);								
		}
		head = head->next;
	}					
	//send response to the place from where this comes
	Message * response = (Message*)malloc(sizeof(Message));
	if (response == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}
	uint32_t loc;
	memcpy(&(loc), data, 4);	
	prepareJoinResponse(response, msg->uoid, loc);
	printJoinResponse(response,  response->data, other->nodeId, 1);
	pthread_mutex_lock(&(other->nodeLock));				
	Append(&(other->writeQhead), &(other->writeQtail), response);
	other->qlen++;
	pthread_mutex_unlock(&(other->nodeLock));
	pthread_cond_signal(&(other->nodeCond));
}
/*
This method will intiated the join process and flood the initial
join packets.
*/
int dojoin(){		
	pthread_mutex_lock(&(joinDBLock));				
	joinDBlen = 0;				
	pthread_mutex_unlock(&(joinDBLock));				

	char inst_id[512];	
	int8_t uoid[20];
	memset(inst_id, 0, 512);	
	memset(uoid, 0, 20);
	getNodeInstId(inst_id, mynodeId);
	GetUOID(inst_id, "msg", uoid, 20);	
	memset(joinuoid, 0, 20);
	memcpy(joinuoid, uoid, 20);
			
	Message* join = (Message*)malloc(sizeof(Message));
	prepareJoin(join, uoid);
	int sock_id;	
	ListElement* head = beaconshead;
	Node* bnode = NULL;
	while(head != NULL){					
		bnode = (Node*)head->item;		
		struct sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(inet_addr(getIPAddr(bnode->hostname)));		
		serv_addr.sin_port = htons(bnode->port);
		memset(&(serv_addr.sin_zero), 0, 8);		
		sock_id = socket(AF_INET, SOCK_STREAM, 0);
		if(connect(sock_id, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) >= 0){					
			if(sendMessage(sock_id, join, bnode->nodeId) == 0){											
				printJoin(join, join->data, bnode->nodeId, 1);
				break;
			}
		}	
		head = head->next;
	}
	if(head == NULL){
		printf("\nNo Beacon Node Found. Quitting!\n");		
		return -1;
	}					
	pthread_create(&(joinThread), NULL, joinHandler, (void*)sock_id);
	Message msg;
	unsigned char temp_buf[512];
	for(;;){
		if(isShutdown || softshutdown){
			break;
		}
		memset(temp_buf, 0, 512);
		if(readByte(sock_id, temp_buf, 27) != 27){	
			//if(!isJoinOn || isShutdown) {	
				shutdown(sock_id, SHUT_RDWR);
				close(sock_id);
				break;	
			//}				
		}
		readMessage(&msg, temp_buf);
		if(msg.type == JOINRESPONSE){
			memset(temp_buf, 0, 512);
			if(readByte(sock_id, temp_buf, msg.dataLength) != msg.dataLength){
				//if(!isJoinOn || isShutdown) {	
					shutdown(sock_id, SHUT_RDWR);
					close(sock_id);
					break;	
				//}				
			}
			printJoinResponse(&msg, temp_buf, bnode->nodeId, 0);
			storeJoinInfoNgbr(msg.dataLength, temp_buf);				
		}
	}
	int status;
	pthread_join(joinThread, (void*)&status);
	if(joinSuccessfull){
		return 0;
	}
	else{
		return -1;
	}
}

