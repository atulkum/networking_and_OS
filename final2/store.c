#include "store.h"
int sendStore(Node* ngb, uint8_t ttl, char* uoid, char* buffer, int length, int inode, int filesize, int issend){
	Message msg;
	memset(&msg, 0, sizeof(Message));
	msg.type = STORE;
	memset(msg.uoid, 0, 20);
	memcpy(msg.uoid, uoid, 20);
	msg.ttl = ttl;	
	msg.dataLength = 4 + length + filesize;

	unsigned char temp[32];
	memset(temp, 0, 32);
	int off = 0;
	memcpy(temp + off, &(msg.type), 1); off += 1;
	memcpy(temp + off, msg.uoid, 20); off += 20;
	memcpy(temp + off, &(msg.ttl), 1); off += 1;
	off += 1; //reserved
	memcpy(temp + off, &(msg.dataLength), 4); off += 4;	
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

	if(issend) writeLog(SEND, ngb->nodeId, msg.dataLength, ttl, STOR, "", uoid);
	else writeLog(FORW, ngb->nodeId, msg.dataLength, ttl, STOR, "", uoid);
	return 0;
}
void doStore(uint16_t ttltosend, char* filename){
	Metadata mdata;
	int inode = prepareMetadata(&mdata, filename);
	if(inode <= 0){
		return;
	}
	char buffer[512];
	memset(buffer, 0, 512);
	int len = dumpMetadata(&mdata, buffer);
	writeMetadata(buffer, inode);
	updateIndexFiles(&mdata, 0);
	if(ttltosend > 0){						
		char inst_id[128];	
		char uoid[20];
		memset(inst_id, 0, 128);	
		memset(uoid, 0, 20);
		getNodeInstId(inst_id, mynodeId);
		GetUOID(inst_id, "msg", uoid, 20);
		ListElement* head = conhead;
		while(head != NULL){					
			Node* nei = (Node*)head->item;
			if(nei->isHelloDone == 1){
				if(isprobe(ngbrStoreProb)){
					sendStore(nei, ttltosend, uoid, buffer, len, mdata.inodeno, mdata.filesize, 1);						
				}
			}
			head = head->next;
		}
	}
}
int readMetadataAndStoreFile(int sock_id, int datalen, uint32_t* metadataLen, unsigned char* buffer){
	unsigned char temp[512]; 
	memset(temp, 0, 512);	
	if(readByte(sock_id, temp, 4) != 4){
		fprintf(logfile, "** Read error\n");fflush(logfile);		
		return -1;
	}		
	memcpy(metadataLen, temp, 4);	
	memset(temp, 0, 512);	
	if(readByte(sock_id, temp, *metadataLen) != *metadataLen){
		fprintf(logfile, "** Read error\n");fflush(logfile);		
		return -1;
	}
	memcpy(buffer, temp, *metadataLen);
	//get the next inode number, store the metadata and file
	//calculate sha1 and try to compare it with the sha1 in metadata		
	int nextinode = getNextFileInode();
	writeMetadata((char*)temp, nextinode);
	Metadata mdata;	
	readMetadata(&mdata, nextinode);
	updateIndexFiles(&mdata, 1);
	int length = datalen - 4 - *metadataLen;
	char filename[256];
	memset(filename, 0, 256);
	sprintf(filename, "%s/files/%d.data", homeDir, mdata.inodeno);
	FILE* file = fopen (filename, "wb");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file %s open fails\n", filename);fflush(logfile);
		return -1;
	}
	SHA_CTX ctx;
	SHA1_Init(&ctx);	
	unsigned char filebuffer[1024];
	for(;;){
		memset(filebuffer, 0, 1024);
		if(length < 1024){
			if(readByte(sock_id, filebuffer, length) != length){
				fprintf(logfile, "** Read error\n");fflush(logfile);		
				return -1;
			}
			fwrite(filebuffer, 1, length, file);
			SHA1_Update(&ctx, filebuffer, length); 
			break;
		}
		else{
			if(readByte(sock_id, filebuffer, 1024) != 1024){
				fprintf(logfile, "** Read error\n");fflush(logfile);		
				return -1;
			}
			fwrite(filebuffer, 1, 1024, file);
			SHA1_Update(&ctx, filebuffer, 1024);
			length -= 1024;
		}
	}
	fclose(file);
	memset(temp, 0, 512);	
	SHA1_Final(temp, &ctx);
	memset(filename, 0, 256);
	int2hex((unsigned char *)temp, 20, filename);
	if(!strcmp(mdata.sha1, filename)){	
	}	
	return nextinode;
}

