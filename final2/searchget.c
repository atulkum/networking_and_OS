#include "searchget.h"
int isgetfilename = 0;

void processSearch(char* line){	
	char inst_id[512];	
	char uoid[20];
	memset(inst_id, 0, 512);	
	memset(uoid, 0, 20);
	getNodeInstId(inst_id, mynodeId);
	GetUOID(inst_id, "msg", uoid, 20);
	memset(statusuoid, 0, 20);
	memcpy(statusuoid, uoid, 20);

	char *key, *value;
	GetKeyValue(line, '=', &key, &value);
	uint8_t searchtype = 0;
	int nfile = 0;
	int *inodes = NULL;
	int status = 0;	
	if(!strcmp(key, "filename")){		
		searchtype = 1;
		status = searchName(value, &inodes, &nfile);
	}
	else if(!strcmp(key, "sha1hash")){
		searchtype = 2;
		status = searchSha1(value, &inodes, &nfile);
	}
	else if(!strcmp(key, "keywords")){
		searchtype=3;		
		if(value[0] == '"'){
			value++;
			value[strlen(value)-1] = '\0';
		}
		status = searchBitVector(value, &inodes, &nfile);
	}
	pthread_mutex_lock(&(statusDBLock));								
	statusDBlen = 0;							
	ListElement *head = fileidcachehead;	
	while(head != NULL){
		InodeUoid* i2u = (InodeUoid*)head->item;
		free(i2u);
		head = head->next;
	}
	destroyList(&fileidcachehead, &fileidcachetail);

	if(status == 0){
		updateLRUonSearch(inodes, nfile);
		char buffer[512];
		int i;
		for(i = 0; i < nfile; ++i){
			statusDBlen++;
			//createfileid
			InodeUoid *i2u = getFileId(inodes[i]);
			if(i2u == NULL){
				i2u = (InodeUoid*)malloc(sizeof(InodeUoid));
				if (i2u == NULL) {
					fprintf(logfile, "** malloc() failed\n");fflush(logfile);
				}	
				memset(i2u, 0, sizeof(InodeUoid));
				GetUOID(inst_id, "fileid", i2u->uoid, 20);						
				i2u->inode = inodes[i];
				Append(&fileidhead, &fileidtail, i2u);
			}
			InodeUoid* cashedi2u = (InodeUoid*)malloc(sizeof(InodeUoid));
			if (cashedi2u == NULL) {
				fprintf(logfile, "** malloc() failed\n");fflush(logfile);
			}	
			memset(cashedi2u, 0, sizeof(InodeUoid));
			memcpy(cashedi2u->uoid,  i2u->uoid, 20); 
			cashedi2u->inode = statusDBlen;
			Append(&fileidcachehead, &fileidcachetail, cashedi2u);
		
			memset(buffer, 0, 512);
			int2hex((unsigned char *)cashedi2u->uoid, 20, buffer);
			printf("[%d] FileID=%s\n", statusDBlen, buffer);
			memset(buffer, 0, 512);
			readMetadataFromFile4Search(buffer, inodes[i]);						
			printf("%s", buffer);
		}	
	}
	pthread_mutex_unlock(&(statusDBLock));
	head = conhead;
	while(head != NULL){					
		Node* nei = (Node*)head->item;
		if(nei->isHelloDone == 1){
			Message * search = (Message*)malloc(sizeof(Message));
			if (search == NULL) {
				fprintf(logfile, "** malloc() failed\n");fflush(logfile);
			}
			prepareSearch(search, uoid, searchtype, value);		
			char toprint[256];
			memset(toprint, 0, 256);
			sprintf(toprint, "%d %s", searchtype, value);
			writeLog(SEND, nei->nodeId, search->dataLength, search->ttl, SHRQ, toprint, search->uoid);
		
			pthread_mutex_lock(&(nei->nodeLock));				
			Append(&(nei->writeQhead), &(nei->writeQtail), search);
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
void prepareSearch(Message *msg, char* uoid, uint8_t type, char* value){
	memset(msg, 0, sizeof(Message));
	msg->type = SEARCH;
	memset(msg->uoid, 0, 20);
	memcpy(msg->uoid, uoid, 20);
	msg->ttl = ttl;	
	msg->dataLength = 1 + strlen(value);	
	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	memcpy(msg->data, &(type), 1);
	memcpy(msg->data + 1, value, strlen(value));
}

void sendSearchResponse(char* uoid, Node* ngb, uint8_t searchtype, char* value){
	int nfile = 0;
	int *inodes = NULL;
	int status = 0;
	switch(searchtype){
		case 1: status = searchName(value, &inodes, &nfile);
			break;
		case 2: status = searchSha1(value, &inodes, &nfile);
			break;
		case 3: status = searchBitVector(value, &inodes, &nfile);
			break;
	}
	if(status != 0){
		return;
	}
	updateLRUonSearch(inodes, nfile);

	Message *msg = (Message*)malloc(sizeof(Message));
	if(msg == NULL){
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}
	memset(msg, 0, sizeof(Message));
	msg->type = SEARCHRESPONSE;
	char inst_id[512];	
	memset(inst_id, 0, 512);	
	memset(msg->uoid, 0, 20);
	getNodeInstId(inst_id, mynodeId);
	GetUOID(inst_id, "msg", msg->uoid, 20);
	msg->ttl = 1;	

	uint32_t length = 0;
	Searchinfo *info = (Searchinfo *)malloc(sizeof(Searchinfo)*nfile);
	if (info == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	memset(info, 0, sizeof(Searchinfo)*nfile);			
	int i;
	for(i = 0; i < nfile; ++i){			
		info[i].reclen = readMetadataFromFile4Search(info[i].buffer, inodes[i]);			
		length += info[i].reclen + 4 + 20;		
		//createfileid
		InodeUoid *i2u = getFileId(inodes[i]);
		if(i2u == NULL){
			i2u = (InodeUoid*)malloc(sizeof(InodeUoid));
			if (i2u == NULL) {
				fprintf(logfile, "** malloc() failed\n");fflush(logfile);
			}	
			memset(i2u, 0, sizeof(InodeUoid));
			GetUOID(inst_id, "fileid", i2u->uoid, 20);						
			i2u->inode = inodes[i];
			Append(&fileidhead, &fileidtail, i2u);
		}
		memset(info[i].uoid, 0, 20);
		memcpy(info[i].uoid, i2u->uoid, 20);
	}	
	msg->dataLength = 20 + length;
	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	int off = 0;
	memcpy(msg->data + off, uoid, 20); off += 20;
	for( i = 0; i < nfile; ++i){			
		memcpy(msg->data + off, &(info[i].reclen), 4); off += 4;
		memcpy(msg->data + off, info[i].uoid, 20); off += 20;
		memcpy(msg->data + off, info[i].buffer, info[i].reclen); off += info[i].reclen;
	}
	free(info);
	char toprint[9];
	int2hex((unsigned char*)(uoid + 16), 4, toprint);				
	writeLog(SEND, ngb->nodeId, msg->dataLength, msg->ttl, SHRS, toprint, msg->uoid);	
	pthread_mutex_lock(&(ngb->nodeLock));				
	Append(&(ngb->writeQhead), &(ngb->writeQtail), msg);
	ngb->qlen++;
	pthread_cond_signal(&(ngb->nodeCond));
	pthread_mutex_unlock(&(ngb->nodeLock));
}

void processSearchResponse(int len, unsigned char* data){
	int off = 20; //for uoid
	uint32_t mdatalen;
	char buffer[512];
	pthread_mutex_lock(&(statusDBLock));				
	while(off != len){
		statusDBlen++;
		memcpy(&mdatalen, data + off, 4); off += 4;
		InodeUoid* i2u = (InodeUoid*)malloc(sizeof(InodeUoid));
		if (i2u == NULL) {
			fprintf(logfile, "** malloc() failed\n");fflush(logfile);
		}	
		memset(i2u, 0, sizeof(InodeUoid));
		memcpy(i2u->uoid, data + off, 20); off += 20;
		i2u->inode = statusDBlen;
		Append(&fileidcachehead, &fileidcachetail, i2u);
		
		memset(buffer, 0, 512);
		int2hex((unsigned char *)i2u->uoid, 20, buffer);
		printf("[%d] FileID=%s\n", statusDBlen, buffer);
		
		memset(buffer, 0, 512);			
		memcpy(buffer, data + off,  mdatalen); off += mdatalen;
		buffer[mdatalen] = '\0';		
		printf("%s", buffer);			
	}
	pthread_mutex_unlock(&(statusDBLock));
}

void processGet(char* filename, int id){	
	if(filename != NULL){
		isgetfilename = 1;
		memset(statusFilename, 0, 512);
		strcpy(statusFilename, filename);
	}
	else{
		isgetfilename = 0;
		memset(statusFilename, 0, 512);
	}	
	InodeUoid* toget = NULL;
	
	ListElement *head = fileidcachehead;	
	while(head != NULL){
		InodeUoid* i2u = (InodeUoid*)head->item;
		if(i2u->inode == id){
			toget = i2u;
			break;
		}
		head = head->next;
	}
	if(	toget == NULL){
		printf("No search entry found\n");
		return;
	}	
	int inode = isFileIdExists(toget->uoid);	
	if( inode > 0){
		//file exist on the same file system , put into permanent space
		removefromcache(inode);
		writeAfterGet(inode);
		return;
	}
	else{
		//flood get message	
		char inst_id[512];	
		char uoid[20];
		memset(inst_id, 0, 512);	
		memset(uoid, 0, 20);
		getNodeInstId(inst_id, mynodeId);
		GetUOID(inst_id, "msg", uoid, 20);
		memset(statusuoid, 0, 20);
		memcpy(statusuoid, uoid, 20);
		head = conhead;
		while(head != NULL){					
			Node* nei = (Node*)head->item;
			if(nei->isHelloDone == 1){
				Message * get = (Message*)malloc(sizeof(Message));
				if (get == NULL) {
					fprintf(logfile, "** malloc() failed\n");fflush(logfile);
				}
				prepareGet(get, uoid, toget->uoid);		
				char toprint[41];
				memset(toprint, 0, 41);
				int2hex((unsigned char*)toget->uoid, 20, (char*)toprint);
				writeLog(SEND, nei->nodeId, get->dataLength, get->ttl, GTRQ, toprint, get->uoid);
				pthread_mutex_lock(&(nei->nodeLock));				
				Append(&(nei->writeQhead), &(nei->writeQtail), get);
				nei->qlen++;
				pthread_mutex_unlock(&(nei->nodeLock));
				pthread_cond_signal(&(nei->nodeCond));			
			}
			head = head->next;
		}
	}
	struct timeval delay;
	delay.tv_sec = getMsgLifeTime;
	delay.tv_usec = 0;
	select(0, NULL, NULL, NULL, &delay);
}

void writeAfterGet(int inode){
	Metadata mdata;		
	readMetadata(&mdata, inode);

	if(isgetfilename == 0){
		strcpy(statusFilename, mdata.filename );
	}
	//see if file already exists , if yes ask the user for overwriting
	struct stat stFileInfo;
	char string[256];
	int iswrite = 1;
	if(stat(statusFilename, &stFileInfo) == 0){
		printf ("\nFile %s already exists. Do you want to overwrite(y/n)?", statusFilename);			
		memset(string, 0, 256);
		char* temp = gets(string);
		if(temp != NULL && (string[0] == 'y' || string[0] == 'Y')){
			iswrite = 1;
		}	
		else{
			iswrite = 0;
		}
	}
	if(iswrite){
		memset(string, 0, 256);
		sprintf(string, "%s/files/%d.data", homeDir, inode);
		FILE* to = fopen (statusFilename, "wb");
		if(to == NULL){
			fprintf(logfile, "** ERROR: file %s open fails\n", statusFilename);fflush(logfile);
		}
		FILE* from = fopen (string, "rb");
		if(from == NULL){
			fprintf(logfile, "** ERROR: file %s open fails\n", string);fflush(logfile);
		}				
		char buffer[1024];
		int len;
		while(!feof(from)){
			memset(buffer, 0, 1024);
			len = fread(buffer, 1, 1024, from);			
			fwrite(buffer, 1, len, to);										
		}
		fclose(to);
		fclose(from);
	}
	Metadata mdata2;
	//consistency check
	ListElement *head = inodehead;
	while(head != NULL){
		Inode *inodenum = (Inode*) head->item;
		readMetadata(&mdata2, inodenum->inum);
		if(!strcmp(mdata.nonce, mdata2.nonce) && (mdata.inodeno != mdata2.inodeno)){			
			deleteFiles(&mdata);
			break;
		}		
		head = head->next;
	}
}

void prepareGet(Message *msg, char* uoid, char *fileid){
	memset(msg, 0, sizeof(Message));
	msg->type = GET;
	memset(msg->uoid, 0, 20);
	memcpy(msg->uoid, uoid, 20);
	msg->ttl = ttl;	
	msg->dataLength = 40;	
	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	char fileidhash[20];
	memset(fileidhash, 0, 20);
	SHA1((unsigned char*)fileid, 20, (unsigned char*)fileidhash);		

	memcpy(msg->data, fileid, 20);
	memcpy(msg->data + 20, fileidhash, 20);
}

void sendGetResponse(char *uoid, Node* ngb, int inode){
	Metadata mdata;	
	readMetadata(&mdata, inode);
	char buffer[512];
	memset(buffer, 0, 512);
	int len = dumpMetadata(&mdata, buffer);
	sendGet(ngb, uoid, buffer, len, inode, mdata.filesize, 1);	
}
int sendGet(Node* ngb, char* uoid, char* buffer, int length, int inode, int filesize, int issend){	
	Message msg;
	memset(&msg, 0, sizeof(Message));
	msg.type = GETRESPONSE;
	memset(msg.uoid, 0, 20);
	char inst_id[512];	
	memset(inst_id, 0, 512);	
	getNodeInstId(inst_id, mynodeId);
	GetUOID(inst_id, "msg", msg.uoid, 20);
	msg.ttl = 1;	
	msg.dataLength = 20 + 4 + length + filesize;

	unsigned char temp[64];
	memset(temp, 0, 64);
	int off = 0;
	memcpy(temp + off, &(msg.type), 1); off += 1;
	memcpy(temp + off, msg.uoid, 20); off += 20;
	memcpy(temp + off, &(msg.ttl), 1); off += 1;
	off += 1; //reserved
	memcpy(temp + off, &(msg.dataLength), 4); off += 4;	
	memcpy(temp + off, uoid, 20); off += 20;	
	memcpy(temp + off, &(length), 4); off += 4;		
	//we need a lock here for writing	
	pthread_mutex_lock(&ngb->nodeLock);
	if(writeByte(ngb->sock_id, temp, off) != off) {
		fprintf(logfile, "** Write error\n");fflush(logfile);		
		return -1;
	}	
	if(writeByte(ngb->sock_id, (unsigned char*)buffer, length) != length) {
		fprintf(logfile, "** Write error\n");fflush(logfile);		
		return -1;
	}		
	char filename[256];
	memset(filename, 0, 256);
	sprintf(filename, "%s/files/%d.data", homeDir, inode);
	FILE* file = fopen (filename, "rb");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file %s open fails\n", filename);fflush(logfile);
		return -1;
	}	
	int len;
	unsigned char filebuffer[1024];
	while(!feof(file)){
		memset(filebuffer, 0, 1024);
		len = fread(filebuffer, 1, 1024, file);
		if(writeByte(ngb->sock_id, filebuffer, len) != len) {
			fprintf(logfile, "** Write error\n");fflush(logfile);		
			return -1;
		}
	}
	fclose(file);	
	pthread_mutex_unlock(&ngb->nodeLock);

	char toprint[41];
	memset(toprint, 0, 41);
	int2hex((unsigned char*)uoid, 20, toprint);
	
	if(issend) writeLog(SEND, ngb->nodeId, msg.dataLength, 1, GTRS, toprint, msg.uoid);
	else writeLog(FORW, ngb->nodeId, msg.dataLength, 1, GTRS, toprint, msg.uoid);
	return 0;
}
