#include "status.h"

/*
Flags and data structure for status message type.
*/
int isFile = 0; //1 file, 0 neighbor

char statusuoid[20];
StatusNode statusDB[10];
int statusDBlen = 0;

pthread_mutex_t statusDBLock;
char statusFilename[512];

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
void statusHandler(){	
	struct timeval delay;
	delay.tv_sec = msgLifeTime;
	delay.tv_usec = 0;
	select(0, NULL, NULL, NULL, &delay);

	pthread_mutex_lock(&(statusDBLock));				
	int index = statusDBlen++;
	statusDB[index].thisport = myport;
	int inei = 0;
	ListElement* head = conhead;			
	while(head != NULL){			
		Node* nei = (Node*)head->item;
		if(nei->isHelloDone == 1){				
			statusDB[index].neibr[inei] = nei->port;
			inei++;
		}
		head = head->next;
	}		
	statusDB[index].nnei = inei;

	uint16_t seen[10]; 
	int seeni = 0;
	
	//print the packets
	int i, j, k;
	FILE* file = fopen (statusFilename, "w");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file init_neighbor_list open fails\n");fflush(logfile);
	}							
	fprintf(file, "V -t * -v 1.0a5\n");	
	int isseen = 0;
	for(i = 0 ;i < statusDBlen; ++i){
		isseen = 0;
		for(k = 0 ; k < seeni; ++k){
			if(seen[k] == statusDB[i].thisport) {
				isseen = 1;
				break;
			}
		}
		if(!isseen){
			fprintf(file, "n -t * -s %d -c red -i black\n",  statusDB[i].thisport);
			seen[seeni++] = statusDB[i].thisport;
		}
		for(j = 0; j < statusDB[i].nnei; ++j){
			isseen = 0;
			for(k = 0 ; k < seeni; ++k){
				if(seen[k] == statusDB[i].neibr[j]) {
					isseen = 1;
					break;
				}
			}
			if(!isseen){
				fprintf(file, "n -t * -s %d -c red -i black\n",  statusDB[i].neibr[j]);
				seen[seeni++] = statusDB[i].neibr[j];
			}
			fprintf(file, "l -t * -s %d -d %d -c blue\n",  statusDB[i].thisport, statusDB[i].neibr[j]);
		}
	}	
	fclose(file);
	pthread_mutex_unlock(&(statusDBLock));				
}
/*
This method will build the status message to send.
*/
void prepareStatus(Message *msg, uint8_t ttl, char* uoid, uint8_t type){
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
	memcpy(msg->data, &(type), msg->dataLength); 
}
/*
This methos prepare the response of the status message and flood thme
to the network.
*/
void sendStatusResponse(char* uoid, Node* ngb){
	uint32_t length = 20 + 2 + 2 + strlen(myhostname); //uoid + record length + port + hostname

	ListElement* head = conhead;					
	while(head != NULL){					
		Node* nei = (Node*)head->item;
		if(nei->isHelloDone == 1){
			length += 2 + 2 + strlen(nei->hostname); //record length + port + hostname
		}
		head = head->next;
	}					
			
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

	uint16_t recordlen = 2 + strlen(myhostname);
	memcpy(msg->data + off, &(recordlen), 2); off += 2;
	memcpy(msg->data + off, &(myport), 2); off += 2;		
	memcpy(msg->data + off, myhostname, strlen(myhostname)); off += strlen(myhostname);

	head = conhead;			
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
		
	char toprint[9];
	int2hex((unsigned char*)(uoid + 16), 4, toprint);				
	writeLog(SEND, ngb->nodeId, msg->dataLength, msg->ttl, STRS, toprint, msg->uoid);	
	pthread_mutex_lock(&(ngb->nodeLock));				
	Append(&(ngb->writeQhead), &(ngb->writeQtail), msg);
	ngb->qlen++;
	pthread_cond_signal(&(ngb->nodeCond));
	pthread_mutex_unlock(&(ngb->nodeLock));
}

void sendStatusFileResponse( char* uoid, Node* ngb){
	Message * msg = (Message*)malloc(sizeof(Message));
	if(msg == NULL){
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
	//calculating the total length
	int nfile = 0;
	ListElement* head = inodehead;
	while(head != NULL){
		nfile++;
		head = head->next;
	}
	uint32_t length = 0;
	Metainfo *info = (Metainfo *)malloc(sizeof(Metainfo)*nfile);
	if (info == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	memset(info, 0, sizeof(Metainfo)*nfile);		
	head = inodehead;
	int i = 0;
	while(head != NULL){
		Inode *inodenum = (Inode*) head->item;
		info[i].reclen = readMetadataFromFile(info[i].buffer, inodenum->inum);			
		length += info[i].reclen + 4;		
		++i;
		head = head->next;
	}
	msg->dataLength = 20 + 2 + 2 + strlen(myhostname) + length;
	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	int off = 0;
	memcpy(msg->data + off, uoid, 20); off += 20;
	uint16_t recordlen = 2 + strlen(myhostname);
	memcpy(msg->data + off, &(recordlen), 2); off += 2;
	memcpy(msg->data + off, &(myport), 2); off += 2;		
	memcpy(msg->data + off, myhostname, strlen(myhostname)); off += strlen(myhostname);	
	for( i = 0; i < nfile; ++i){		
		memcpy(msg->data + off, &(info[i].reclen), 4); off += 4;
		memcpy(msg->data + off, info[i].buffer, info[i].reclen); off += info[i].reclen;
	}
	free(info);
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
void storeStatusInfo(int len, unsigned char* data){
	char otherhost[512];
	uint16_t otherport;
	uint16_t reclen;
	int off = 20; //for uoid
	memcpy(&reclen, data + off, 2); off += 2;
	memcpy(&otherport, data + off, 2); off += 2;
	memset(otherhost,0,512);
	memcpy(otherhost, data + off,  reclen - 2); off += reclen - 2;
	otherhost[reclen - 2] = '\0';
	if(!isFile){
		pthread_mutex_lock(&(statusDBLock));				
		int index = statusDBlen++;
		pthread_mutex_unlock(&(statusDBLock));				
		statusDB[index].thisport = otherport;
		int inei = 0;
		while(off != len){
			memcpy(&reclen, data + off, 2); off += 2;
			memcpy(&otherport, data + off, 2); off += 2;
			memset(otherhost, 0, 512);
			memcpy(otherhost, data + off,  reclen - 2); off += reclen - 2;
			otherhost[reclen - 2] = '\0';
			statusDB[index].neibr[inei] = otherport;					
			inei++;
		}
		statusDB[index].nnei = inei;
	}
	else{
		uint32_t mdatalen;
		char buffer[512];
		pthread_mutex_lock(&(statusDBLock));				
		FILE* file = fopen (statusFilename, "a");
		if(file == NULL){
			fprintf(logfile, "** ERROR: status output file open fails\n");fflush(logfile);
		}
		if(off == len){
			fprintf(file, "%s:%d has no file\n", otherhost, otherport);
		}
		else{
			memcpy(&mdatalen, data + off, 4);
			if((off + 4 + mdatalen) == len){
				fprintf(file, "%s:%d has the following file\n", otherhost, otherport);
			}
			else{
				fprintf(file, "%s:%d has the following files\n", otherhost, otherport);
			}
		}		
		while(off != len){
			memcpy(&mdatalen, data + off, 4); off += 4;
			memset(buffer, 0, 512);			
			memcpy(buffer, data + off,  mdatalen); off += mdatalen;
			buffer[mdatalen] = '\0';
			fprintf(file, "%s", buffer);			
		}
		fclose(file);
		pthread_mutex_unlock(&(statusDBLock));
	}
}
void doStatusNgbr(uint16_t ttltosend){
	if(ttltosend == 0){
		//write the information about the node 
		FILE* file = fopen (statusFilename, "w");
		if(file == NULL){
			fprintf(logfile, "** ERROR: status output file open fails\n");fflush(logfile);
		}							
		fprintf(file, "V -t * -v 1.0a5\n");	
		fprintf(file, "n -t * -s %d -c red -i black\n", myport);					
		fclose(file);
	}
	else{
		pthread_mutex_lock(&(statusDBLock));						
		isFile = 0;
		statusDBlen = 0;							
		pthread_mutex_unlock(&(statusDBLock));				
		char inst_id[512];	
		char uoid[20];
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
				prepareStatus(status, ttltosend, uoid, STATUS_TYPE_NEIGHBORS);						
				writeLog(SEND, nei->nodeId, status->dataLength, status->ttl, STRQ, "neighbors", status->uoid);
			
				pthread_mutex_lock(&(nei->nodeLock));				
				Append(&(nei->writeQhead), &(nei->writeQtail), status);
				nei->qlen++;
				pthread_mutex_unlock(&(nei->nodeLock));
				pthread_cond_signal(&(nei->nodeCond));			
			}
			head = head->next;
		}
		statusHandler();
	}
}

void doStatusFile(uint16_t ttltosend){
	FILE* file = fopen (statusFilename, "w");
	if(file == NULL){
		fprintf(logfile, "** ERROR: status output file open fails\n");fflush(logfile);
	}	
	char buffer[512];
	ListElement* head = inodehead;
	if(head == NULL){
		fprintf(file, "%s:%d has no file\n", myhostname, myport);
	}
	else{
		if(head->next == NULL){
			fprintf(file, "%s:%d has the following file\n", myhostname, myport);
		}
		else{
			fprintf(file, "%s:%d has the following files\n", myhostname, myport);
		}
	}	
	while(head != NULL){
		Inode *inodenum = (Inode*) head->item;
		memset(buffer, 0, 512);
		readMetadataFromFile(buffer, inodenum->inum);				
		fprintf(file, "%s", buffer);
		head = head->next;
	}
	fclose(file);
	
	if(ttltosend != 0){
		pthread_mutex_lock(&(statusDBLock));				
		isFile = 1;
		pthread_mutex_unlock(&(statusDBLock));		
		char inst_id[512];	
		char uoid[20];
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
					fprintf(logfile, "** malloc() failed\n");fflush(logfile);
				}
				prepareStatus(status, ttltosend, uoid, STATUS_TYPE_FILES);						
				writeLog(SEND, nei->nodeId, status->dataLength, status->ttl, STRQ, "files", status->uoid);
			
				pthread_mutex_lock(&(nei->nodeLock));				
				Append(&(nei->writeQhead), &(nei->writeQtail), status);
				nei->qlen++;
				pthread_mutex_unlock(&(nei->nodeLock));
				pthread_cond_signal(&(nei->nodeCond));			
			}
			head = head->next;
		}
		struct timeval delay;
		delay.tv_sec = msgLifeTime;
		delay.tv_usec = 0;
		select(0, NULL, NULL, NULL, &delay);
	}
}
