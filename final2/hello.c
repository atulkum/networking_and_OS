#include "hello.h"
/*
This will extract the connection information from the node where 
this hello packet has come from.
*/
int processHello(Message* msg, unsigned char* temp, Node* ngbr){
	char otherhost[512];
	uint16_t otherport; 	
	memcpy(&otherport, temp, 2);
	memset(otherhost,0,512);
	memcpy(otherhost, temp + 2, msg->dataLength - 2);
	otherhost[msg->dataLength - 2] = '\0';
			
	ngbr->port = otherport;
	memset(ngbr->hostname, 0, 512);
	strcpy(ngbr->hostname, otherhost);
	memset(ngbr->nodeId, 0, 512);
	getNodeId(ngbr->nodeId, ngbr->hostname, ngbr->port);
	
	printlogHello(msg, temp, ngbr->nodeId, 0);
	
	Node* nei = findNode(ngbr, conhead);
	if((nei != NULL) && (nei->isHelloDone == 1)){		
		return -1;
	}		
	else{
		return 0;
	}
}
/*
This method would build the HELLO message.
*/
void prepareHello(Message *msg){
	memset(msg, 0, sizeof(Message));
	char inst_id[512];	
	memset(inst_id, 0, 512);	
	memset(msg->uoid, 0, 20);
	getNodeInstId(inst_id, mynodeId);
	GetUOID(inst_id, "msg", msg->uoid, 20);
	msg->type = HELLO;
	msg->ttl = 1;	
	msg->dataLength = 2 + strlen(myhostname);	
	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	memcpy(msg->data, &(myport), 2); 
	memcpy(msg->data + 2, myhostname, strlen(myhostname)); 
}
/*
This will send hello message to the beacon node and intiated the connection.
*/
int sendHelloBeacon(Node* nei){	
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(inet_addr(getIPAddr(nei->hostname)));
	serv_addr.sin_port = htons(nei->port);
	memset(&(serv_addr.sin_zero), 0, 8);
	Message* hello = (Message*)malloc(sizeof(Message));
	prepareHello(hello);
	//this is to make sure that port is available for reuse after server shutdown
	for(;;){
		if(nei->terminate || isShutdown){
			return -1;
		}
		Node * other = findNode(nei, conhead);
		if((other != NULL) && (other->isHelloDone == 1)){			
			return -1;
		}		
		nei->sock_id = socket(AF_INET, SOCK_STREAM, 0);
		if(connect(nei->sock_id, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) >= 0){
			if(sendMessage(nei->sock_id, hello, nei->nodeId) == 0){
				nei->isHelloDone = 1;
				break;
			}
		}
		//printf("sleeping %s\n", nei->nodeId);
		sleep(retry);		
	}	
	return 0;
}
/*
This will send hello message to the non beacon node and intiated the connection.
*/
int sendHelloNonBeacon(Node* nei){	
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(inet_addr(getIPAddr(nei->hostname)));
	serv_addr.sin_port = htons(nei->port);
	memset(&(serv_addr.sin_zero), 0, 8);
	Message* hello = (Message*)malloc(sizeof(Message));
	prepareHello(hello);
	//sending hello	
	nei->sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if(connect(nei->sock_id, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) >= 0){					
		if(sendMessage(nei->sock_id, hello, nei->nodeId) == 0){
			nei->isHelloDone = 1;			
			return 0;
		}
	}				
	return -1;
}


