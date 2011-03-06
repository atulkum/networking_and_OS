#include "files.h"

ListElement* kwrdhead=NULL;
ListElement* kwrdtail=NULL;

ListElement* namehead=NULL;
ListElement* nametail=NULL;

ListElement* sha1head=NULL;
ListElement* sha1tail=NULL;

ListElement* fileidhead=NULL;
ListElement* fileidtail=NULL;

ListElement* inodehead=NULL;
ListElement* inodetail=NULL;

ListElement* fileidcachehead=NULL;
ListElement* fileidcachetail=NULL;

ListElement* lruhead=NULL;
ListElement* lrutail=NULL;

int getNextFileInode(){
	char nextInodeNo[256];
	memset(nextInodeNo, 0, 256);
	sprintf(nextInodeNo, "%s/nextInodeNo", homeDir);
	int inodenum;
	struct stat stFileInfo;
	if(stat(nextInodeNo, &stFileInfo) != 0){
		FILE* inodefile = fopen (nextInodeNo, "w");
		if(inodefile == NULL){
			fprintf(logfile, "** ERROR: file %s open fails\n", nextInodeNo);fflush(logfile);
			return -1;
		}
		fprintf(inodefile, "1");
		fclose(inodefile);
		inodenum = 1;
	}
	else{
		FILE* inodefile = fopen (nextInodeNo, "r");
		if(inodefile == NULL){
			fprintf(logfile, "** ERROR: file %s open fails\n", nextInodeNo);fflush(logfile);
			return -1;
		}
		char line[64];
		memset(line, 0, 64);
		fgets(line, 64, inodefile);
		inodenum = atoi(line);
		inodenum++;
		fclose(inodefile);		
		inodefile = fopen (nextInodeNo, "w");
		fprintf(inodefile, "%d", inodenum);
		fclose(inodefile);		
	}
	return inodenum;
}
int writeMetadata(char *buffer, int inode){
	char filename[256];
	memset(filename, 0, 256);
	sprintf(filename, "%s/files/%d.meta", homeDir, inode);

	FILE* file = fopen (filename, "w");
	fprintf(file, "%s", buffer);
	fclose(file);
	return 0;
}

int prepareMetadata(Metadata *mdata, char *storedfilename){				
	mdata->inodeno = getNextFileInode();
	mdata->nkeys = 0;
	memset(mdata->filename, 0, 256);
	char *temp = strrchr(storedfilename, '/');
	if(temp == NULL){
		strcpy(mdata->filename, storedfilename);			
	}
	else{
		strcpy(mdata->filename, temp + 1);			
	}
	//keyword
	char *token = strtok(NULL, " ");	
	while(token != NULL){	
		//printf("token %s\n", token);
		int slen = strlen(token);
		int i;		
		for(i=0; i < slen; ++i){
			if(token[i] == '=' || token[i] == '"'){
				token[i] = '\0';
			}
		}
		strcpy(mdata->keywords[mdata->nkeys++], token);			
		int len = strlen(token);
		for(i = len + 1; i < slen; ++i){
			if(token[i] == '\0') continue;
			strcpy(mdata->keywords[mdata->nkeys++], token + i);
			i += strlen(token);
		}		
		token = strtok(NULL, " ");
	}
	//bit vector
	memset(mdata->bitvecSHA1, 0, 64);
	memset(mdata->bitvecMD5, 0, 64);
	int i;	
	for(i = 0; i < mdata->nkeys; ++i){		
		setBitVectors(mdata->bitvecSHA1, mdata->bitvecMD5, mdata->keywords[i]);
	}	
	//password
	char filename[256];
	memset(filename, 0, 256);
	sprintf(filename, "%s/files/%d.pass", homeDir, mdata->inodeno);
	FILE* password = fopen (filename, "wb");
	if(password == NULL){
		fprintf(logfile, "** ERROR: file %s open fails\n", filename);fflush(logfile);
		return -1;
	}	
	char uoid[20];
	memset(uoid, 0, 20);
	GetUOID(mdata->filename, "password", uoid, 20);
	char toprint[41];
	memset(toprint, 0, 41);
	int2hex((unsigned char*)uoid, 20, toprint);
	fprintf(password, "%s", toprint);
	fclose(password);
	
	//nonce
	memset(uoid, 0, 20);
	SHA1((unsigned char*)toprint, strlen(toprint), (unsigned char*)uoid);		
	memset(mdata->nonce, 0, 41);
	int2hex((unsigned char*)uoid, 20, mdata->nonce);
	mdata->nonce[40] = '\0';

	//file read + sha1		
	memset(filename, 0, 256);
	sprintf(filename, "%s/files/%d.data", homeDir, mdata->inodeno);

	SHA_CTX ctx;
	SHA1_Init(&ctx);	
	//If there is not enough space in the filesystem, this node should not flood store messages.
	//You should tell the user "file not found" or something equivalent.
	FILE* to = fopen (filename, "wb");
	FILE* from = fopen (storedfilename, "rb");
	char buffer[1024];
	int len;
	mdata->filesize = 0;
	while(!feof(from)){
		memset(buffer, 0, 1024);
		len = fread(buffer, 1, 1024, from);
		fwrite(buffer, 1, len, to);
		SHA1_Update(&ctx, buffer, len); 
		mdata->filesize += len;
	}
	memset(uoid, 0, 20);
	SHA1_Final((unsigned char*)uoid, &ctx);	
	memset(toprint, 0, 41);
	int2hex((unsigned char*)uoid, 20, mdata->sha1);
	mdata->sha1[40] = '\0';
	fclose(from);
	fclose(to);	
	return mdata->inodeno;
}
void tolowerCase(char* str, char* lowered){
	int len = strlen(str);
	int i;
	for ( i = 0; i < len; i++){
		lowered[i] = tolower( (unsigned char) str[i] );
	}
	lowered[len] = '\0';
}
void setBitVectors(unsigned char *sha1, unsigned char *md5, char *key){
	unsigned char uoid[20];
	char lowered[64];
	memset(uoid, 0, 20);
	memset(lowered, 0, 64);
	tolowerCase(key, lowered);
	SHA1((unsigned char*)(lowered), strlen(lowered), uoid);		
	int toshift = uoid[19];
	if(uoid[18] % 2 != 0){
		toshift += 256;
	}	
	sha1[63 - toshift/8] |= (1 << (toshift%8));
	memset(uoid, 0, 20);
	MD5((unsigned char*)(lowered), strlen(lowered), uoid);		
	toshift = uoid[15];
	if(uoid[14] % 2 != 0){
		toshift += 256;
	}	
	md5[63 - toshift/8] |= (1 << (toshift%8));
}
int dumpMetadata(Metadata *mdata, char *buffer){	
	int length = 0;
	sprintf(buffer + length, "[metadata]\r\n");  length += strlen(buffer + length);
	sprintf(buffer + length, "FileName=%s\r\n", mdata->filename);length += strlen(buffer + length);	
	sprintf(buffer + length, "FileSize=%d\r\n", mdata->filesize);length += strlen(buffer + length);
	sprintf(buffer + length, "SHA1=%s\r\n", mdata->sha1);length += strlen(buffer + length);
	sprintf(buffer + length, "Nonce=%s\r\n", mdata->nonce);length += strlen(buffer + length);
	sprintf(buffer + length, "Keywords=");length += strlen(buffer + length);
	int i;	
	for(i = 0; i < mdata->nkeys; ++i){
		sprintf(buffer + length, "%s ", mdata->keywords[i]); length += strlen(buffer + length);
	}
	sprintf(buffer + length, "\r\n"); length += 2;
	char bvs[129];
	char bvm[129];
	memset(bvs, 0, 129);
	memset(bvm, 0, 129);
	int2hex(mdata->bitvecSHA1, 64, bvs);
	int2hex(mdata->bitvecMD5, 64, bvm);

	sprintf(buffer + length, "Bit-vector=%s%s\r\n", bvs, bvm);length += strlen(buffer + length);
	return length;
}
void updateIndexFiles(Metadata *mdata, int iscache){
	ListElement* head = NULL;
	head = kwrdhead;
	while(head != NULL){
		kwrdIndex *kwrd = (kwrdIndex*) head->item;		
		if(kwrd->n > 0) {
			if (!memcmp(mdata->bitvecSHA1, kwrd->bitvecSHA1, 64) && !memcmp(mdata->bitvecMD5, kwrd->bitvecMD5, 64)){
				kwrd->inodeno[kwrd->n++] = mdata->inodeno;
				break;
			}
		}
		head = head->next;
	}
	if(head == NULL){
		kwrdIndex * kwrd = (kwrdIndex*)malloc(sizeof(kwrdIndex));
		if (kwrd == NULL) {
			fprintf(logfile, "** malloc() failed\n");fflush(logfile);
		}
		kwrd->n = 0;
		kwrd->inodeno[kwrd->n++] = mdata->inodeno;
		memcpy(kwrd->bitvecSHA1, mdata->bitvecSHA1, 64);
		memcpy(kwrd->bitvecMD5, mdata->bitvecMD5, 64);
		Append(&kwrdhead, &kwrdtail, kwrd);
	}

	head = namehead;
	int done = 0;
	ListElement* insertpos = NULL;
	while(head != NULL){
		nameIndex * name = (nameIndex*) head->item;
		if(name->n > 0){
			int ret = strcmp(mdata->filename, name->filename); 
			if(ret == 0){
				name->inodeno[name->n++] = mdata->inodeno; 
				done = 1;
				break;
			}
		}
		head = head->next;
	}
	if(done == 0){
		nameIndex * name = (nameIndex*) malloc(sizeof(nameIndex));
		if (name == NULL) {
			fprintf(logfile, "** malloc() failed\n");fflush(logfile);
		}
		name->n = 0;
		name->inodeno[name->n++] = mdata->inodeno;
		strcpy(name->filename, mdata->filename);
		Append(&namehead, &nametail, name);
	}
	head = sha1head;
	done = 0;
	insertpos = NULL;
	while(head != NULL){
		sha1Index * s = (sha1Index*) head->item;
		if(s->n > 0){			
			int ret = strcmp(mdata->sha1, s->sha1);
			if(ret == 0){
				s->inodeno[s->n++] = mdata->inodeno;
				done = 1;
				break;
			}
		}
		head = head->next;
	}
	if(done == 0){
		sha1Index * s = (sha1Index*) malloc(sizeof(sha1Index));
		if (s == NULL) {
			fprintf(logfile, "** malloc() failed\n");fflush(logfile);
		}
		s->n = 0;
		s->inodeno[s->n++] = mdata->inodeno;
		strcpy(s->sha1, mdata->sha1);
		Append(&sha1head, &sha1tail, s);		
	}
	Inode *inodenum = (Inode*)malloc(sizeof(Inode));
	inodenum->inum = mdata->inodeno;
	Append(&inodehead, &inodetail, inodenum);	
	if(iscache){
		//take the caching decision here
		//if current cache size + file size > cache size keep on evicting
	/*	int size = 0;
		head = lruhead;
		Metadata meta;
		while(head != NULL){
			Inode* i = (Inode*) head->item;
			if(i->inum > 0){
				readMetadata(&meta, i->inum);
				size += meta.filesize;
			}
			head = head->next;
		}
		while(mdata->filesize + size > cacheSize){
			Inode* i = Remove(&lruhead, &lrutail);
			if(i->inum > 0){
				readMetadata(&meta, i->inum);
				size -= meta.filesize;
			}
		}
		inodenum = (Inode*)malloc(sizeof(Inode));
		inodenum->inum = mdata->inodeno;
		Append(&lruhead, &lrutail, inodenum);	*/
	}	
}
void deleteFiles(Metadata *mdata){
	ListElement* head = NULL;
	int i, j;
	head = kwrdhead;
	while(head != NULL){
		kwrdIndex * kwrd = (kwrdIndex*) head->item;	
		if(kwrd->n > 0){
			if (!memcmp(mdata->bitvecSHA1, kwrd->bitvecSHA1, 64) && !memcmp(mdata->bitvecMD5, kwrd->bitvecMD5, 64)){
				if(kwrd->n == 1){
					kwrd->n = 0;
					//removeThis(&kwrdhead, &kwrdtail, head);
					//free(kwrd);
				}
				else{
					for(i = 0; i < kwrd->n; ++i ){
						if(kwrd->inodeno[i] == mdata->inodeno){						
							for(j = i; j < (kwrd->n - 1); ++j){
								kwrd->inodeno[j] = kwrd->inodeno[j + 1];
							}
							kwrd->n--;
							break;
						}
					}				
				}	
				break;
			}
		}
		head = head->next;
	}
	head = namehead;	
	while(head != NULL){
		nameIndex * name = (nameIndex*) head->item;
		if(name->n > 0){
			if(!strcmp(mdata->filename, name->filename)){			
				if(name->n == 1){
					//removeThis(&namehead, &nametail, head);
					//free(name);
					name->n = 0;
				}
				else{
					for(i = 0; i < name->n; ++i ){
						if(name->inodeno[i] == mdata->inodeno){
							for(j = i; j < (name->n - 1); ++j){
								name->inodeno[j] = name->inodeno[j + 1];
							}
							name->n--;
							break;
						}
					}			
				}
				break;
			}
		}
		head = head->next;
	}
	head = sha1head;
	while(head != NULL){
		sha1Index * s = (sha1Index*) head->item;
		if(s->n > 0){
			if(!strcmp(mdata->sha1, s->sha1)){
				if(s->n == 1){
					//removeThis(&sha1head, &sha1tail, head);
					//free(s);
					s->n = 0;
				}
				else{
					for(i = 0; i < s->n; ++i ){
						if(s->inodeno[i] == mdata->inodeno){						
							for(j = i; j < (s->n - 1); ++j){
								s->inodeno[j] = s->inodeno[j + 1];
							}
							s->n--;
							break;
						}
					}			
				}
				break;
			}
		}
		head = head->next;
	}		
	/*head = lruhead;
	while(head != NULL){
		Inode *inodenum = (Inode*) head->item;
		if(inodenum->inum == mdata->inodeno){				
			//removeThis(&lruhead, &lrutail, head);
			inodenum->inum = -1;
			//no free as it would be there in the inodehead too
			break;
		}			
		head = head->next;
	}*/
	head = inodehead;
	while(head != NULL){
		Inode *inodenum = (Inode*) head->item;
		if(inodenum->inum == mdata->inodeno){				
			//removeThis(&inodehead, &inodetail, head);
			//free(inodenum);
			inodenum->inum = -1;
			break;
		}		
		head = head->next;
	}

	head = fileidhead;
	while(head != NULL){
		InodeUoid *i = (InodeUoid*) head->item;						
		if(i->inode == mdata->inodeno){				
			//removeThis(&fileidhead, &fileidtail, head);
			//free(i);
			i->inode = -1;
			break;
		}
		head = head->next;
	}
	char filename[256];
	memset(filename, 0, 256);
	sprintf(filename, "%s/files/%d.pass", homeDir, mdata->inodeno);
	remove(filename);
	memset(filename, 0, 256);
	sprintf(filename, "%s/files/%d.data", homeDir, mdata->inodeno);
	remove(filename);
	memset(filename, 0, 256);
	sprintf(filename, "%s/files/%d.meta", homeDir, mdata->inodeno);
	remove(filename);
}

int dumpIndexFiles(){
	char filename[256];
	ListElement* head = NULL;
	FILE* file = NULL;
	int i;
	memset(filename, 0, 256);
	sprintf(filename, "%s/kwrd_index", homeDir);	
	file = fopen (filename, "w");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file %s open fails\n", filename);fflush(logfile);
		return -1;
	}	
	head = kwrdhead;
	while(head != NULL){		
		kwrdIndex *kwrd = (kwrdIndex*) head->item;
		if(kwrd->n > 0){
			char toprint[128 + 1];
			memset(toprint, 0, 129);
			int2hex(kwrd->bitvecSHA1, 64, toprint);
			fprintf(file, "%s\n", toprint);
			memset(toprint, 0, 129);
			int2hex(kwrd->bitvecMD5, 64, toprint);
			fprintf(file, "%s\n", toprint);
			for( i = 0; i < kwrd->n; ++i){
				if(i == 0) fprintf(file, "%d", kwrd->inodeno[i]);
				else fprintf(file, " %d", kwrd->inodeno[i]);
			}
			fprintf(file, "\n");
		}
		head = head->next;
	}
	fclose(file);

	memset(filename, 0, 256);
	sprintf(filename, "%s/name_index", homeDir);	
	file = fopen (filename, "w");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file %s open fails\n", filename);fflush(logfile);
		return -1;
	}
	head = namehead;
	while(head != NULL){
		nameIndex * name = (nameIndex*) head->item;
		if(name->n > 0){
			fprintf(file, "%s\n", name->filename);			
			for( i = 0; i < name->n; ++i){
				if(i == 0) fprintf(file, "%d", name->inodeno[i]);
				else fprintf(file, " %d", name->inodeno[i]);			
			}
			fprintf(file, "\n");				
		}
		head = head->next;
	}
	fclose(file);

	memset(filename, 0, 256);
	sprintf(filename, "%s/sha1_index", homeDir);	
	file = fopen (filename, "w");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file %s open fails\n", filename);fflush(logfile);
		return -1;
	}	
	head = sha1head;
	while(head != NULL){	
		sha1Index * s = (sha1Index*) head->item;
		if(s->n > 0){
			fprintf(file, "%s\n", s->sha1);
			for( i = 0; i < s->n; ++i){
				if(i == 0) fprintf(file, "%d", s->inodeno[i]);
				else fprintf(file, " %d", s->inodeno[i]);
			}
			fprintf(file, "\n");
		}
		head = head->next;
	}
	fclose(file);	
	return 0;
}
int reloadIndexFiles(){
	char filename[256];
	char line[256];
	Metadata mdata;

	memset(filename, 0, 256);
	sprintf(filename, "%s/name_index", homeDir);	
	struct stat stFileInfo;
	if(stat(filename, &stFileInfo) != 0){
		return -1;
	}
	FILE* file = fopen (filename, "r");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file %s open fails\n", filename);fflush(logfile);
		return -1;
	}	
	memset(line, 0, 256);	
	while(fgets(line, 256, file) != NULL){
		memset(line, 0, 256);
		fgets(line, 256, file);		
		char *token = strtok(line, " ");
		while(token != NULL){
			int inode = atoi(token);			
			readMetadata(&mdata, inode);
			updateIndexFiles(&mdata, 0);
			token = strtok(NULL, " ");
		}
	}	
	fclose(file);
	return 0;
}

int readMetadata(Metadata *mdata, int inum){
	char filename[256];
	memset(filename, 0, 256);
	sprintf(filename, "%s/files/%d.meta", homeDir, inum);
	FILE* file = fopen (filename, "r");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file %s open fails\n", filename);fflush(logfile);
		return -1;
	}	
	mdata->inodeno = inum;

	int lsize = 512;
	char line[512];
	char *key, *value;
	memset(line, 0, lsize);
	fgets(line, lsize, file);		
	if(strcmp(line, "[metadata]"));
	
	memset(line, 0, lsize);
	fgets(line, lsize, file);		
	memset(mdata->filename, 0, 256);
	GetKeyValue(line, '=', &key, &value);
	strcpy(mdata->filename, value);

	memset(line, 0, lsize);	
	fgets(line, lsize, file);		
	GetKeyValue(line, '=', &key, &value);
	mdata->filesize = atoi(value);

	memset(line, 0, lsize);
	fgets(line, lsize, file);		
	memset(mdata->sha1, 0, 41);
	GetKeyValue(line, '=', &key, &value);
	strcpy(mdata->sha1, value);
	
	memset(line, 0, lsize);
	fgets(line, lsize, file);		
	memset(mdata->nonce, 0, 41);
	GetKeyValue(line, '=', &key, &value);
	strcpy(mdata->nonce, value);
	
	memset(line, 0, lsize);
	fgets(line, lsize, file);		
	GetKeyValue(line, '=', &key, &value);
	mdata->nkeys = 0;
	char *token;
	token = strtok(value, " ");
	while(token != NULL){		
		strcpy(mdata->keywords[mdata->nkeys], token);		
		mdata->nkeys++;
		token = strtok(NULL, " ");
	}
	memset(mdata->bitvecSHA1, 0, 64);
	memset(mdata->bitvecMD5, 0, 64);
	memset(line, 0, lsize);
	fgets(line, lsize, file);				
	GetKeyValue(line, '=', &key, &value);

	hex2int(value, mdata->bitvecSHA1, 128);
	hex2int(value + 128, mdata->bitvecMD5, 128);
	fclose(file);
	return 0;
}
int readMetadataFromFile(char* buffer, int inum){
	char filename[256];
	memset(filename, 0, 256);
	sprintf(filename, "%s/files/%d.meta", homeDir, inum);	
	FILE* file = fopen (filename, "r");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file %s open fails\n", filename);fflush(logfile);
		return -1;
	}	
	int length = 0;
	char line[512];
	memset(line, 0, 512);
	while(fgets(line, 512, file) != NULL){
		memcpy(buffer + length, line, strlen(line));  length += strlen(line);		
		memset(line, 0, 512);
	}			
	fclose(file);
	return length;
}

int readMetadataFromFile4Search(char* buffer, int inum){
	char filename[256];
	memset(filename, 0, 256);
	sprintf(filename, "%s/files/%d.meta", homeDir, inum);	
	FILE* file = fopen (filename, "r");
	if(file == NULL){
		fprintf(logfile, "** ERROR: file %s open fails\n", filename);fflush(logfile);
		return -1;
	}	
	int length = 0;
	char line[512];
	memset(line, 0, 512);
	memset(line, ' ', 4);
	int i = 0;
	while(fgets(line + 4, 512, file) != NULL){
		if(i > 0 && i < 6){
			memcpy(buffer + length, line, strlen(line));  length += strlen(line);					
		}
		memset(line, 0, 512);
		memset(line, ' ', 4);
		i++;
	}			
	fclose(file);
	return length;
}
void filecleanup(){
	dumpIndexFiles();
	ListElement* head = NULL;
	
	head = kwrdhead;
	while(head != NULL){
		kwrdIndex *kwrd = (kwrdIndex*) head->item;
		free(kwrd);
		head = head->next;
	}
	destroyList(&kwrdhead, &kwrdtail);
	head = namehead;
	while(head != NULL){
		nameIndex * name = (nameIndex*) head->item;
		free(name);
		head = head->next;
	}
	destroyList(&namehead, &nametail);
	head = sha1head;
	while(head != NULL){
		sha1Index * s = (sha1Index*) head->item;
		free(s);
		head = head->next;
	}
	destroyList(&sha1head, &sha1tail);
	head = inodehead;
	while(head != NULL){
		Inode *inodenum = (Inode*) head->item;
		free(inodenum);
		head = head->next;
	}
	destroyList(&inodehead, &inodetail);
	
	/*head = lruhead;
	while(head != NULL){
		Inode *inodenum = (Inode*) head->item;
		free(inodenum);
		head = head->next;
	}
	destroyList(&lruhead, &lrutail);*/
	
	head = fileidhead;
	while(head != NULL){
		InodeUoid *inodenum = (InodeUoid*) head->item;
		free(inodenum);
		head = head->next;
	}
	destroyList(&fileidhead, &fileidtail);

	head = fileidcachehead;
	while(head != NULL){
		InodeUoid *inodenum = (InodeUoid*) head->item;
		free(inodenum);
		head = head->next;
	}
	destroyList(&fileidcachehead, &fileidcachetail);
}
int searchBitVector(char* key, int** inodes, int* len){	
	char *token = NULL;
	ListElement *head = kwrdhead;
	while(head != NULL){
		unsigned char  flag = 0;
		kwrdIndex *kwrd = (kwrdIndex*) head->item;
		if(kwrd->n > 0){
			token = strtok(key, " ");			
			while(token != NULL){			
				if(!isSet(kwrd->bitvecSHA1, kwrd->bitvecMD5, token)){
					flag = 0;
					break;				
				}
				else{
					token = strtok(NULL, " ");	
					flag = 1;
				}
			}
			if(flag){
				*inodes = kwrd->inodeno;
				*len = kwrd->n;
				return 0;
			}
		}
		head = head->next;
	}
	return -1;
}
int searchName(char* key, int** inodes, int* len){
	ListElement* head = namehead;
	while(head != NULL){
		nameIndex * name = (nameIndex*) head->item;				
		if(name->n > 0){
			if(!strcmp(name->filename, key)){
				*inodes = name->inodeno;
				*len = name->n;
				return 0;
			}
		}
		head = head->next;
	}
	return -1;
}
int searchSha1(char* key, int** inodes, int* len){
	ListElement* head = sha1head;
	while(head != NULL){
		sha1Index * s = (sha1Index*) head->item;						
		if(s->n > 0 ){
			if(!strcmp(s->sha1, key)){
				*inodes = s->inodeno;
				*len = s->n;
				return 0;
			}
		}
		head = head->next;
	}
	return -1;
}
InodeUoid* getFileId(int inode){
	ListElement* head = fileidhead;
	while(head != NULL){
		InodeUoid *i = (InodeUoid*) head->item;						
		if(i->inode == inode){
			return i;
		}
		head = head->next;
	}
	return NULL;
}
int isFileIdExists(char *fileid){
	ListElement* head = fileidhead;
	while(head != NULL){
		InodeUoid *i = (InodeUoid*) head->item;						
		if(!memcmp(i->uoid, fileid, 20)){
			return i->inode;
		}
		head = head->next;
	}
	return -1;
}
unsigned char isSet(unsigned char *sha1, unsigned char *md5, char *key){
	unsigned char uoid[20];
	char lowered[64];
	memset(uoid, 0, 20);
	memset(lowered, 0, 64);
	tolowerCase(key, lowered);
	SHA1((unsigned char*)(lowered), strlen(lowered), uoid);		
	int toshift = uoid[19];
	if(uoid[18] % 2 != 0){
		toshift += 256;
	}	
	unsigned char shift = (1 << (toshift%8));
	if(!(sha1[63 - toshift/8] & shift)){
		return 0;
	}
	memset(uoid, 0, 20);
	MD5((unsigned char*)(lowered), strlen(lowered), uoid);		
	toshift = uoid[15];
	if(uoid[14] % 2 != 0){
		toshift += 256;
	}	
	shift = (1 << (toshift%8));
	if(!(md5[63 - toshift/8] & shift)){
		return 0;
	}
	return 1;
}

void updateLRUonSearch(int* inodes, int n){
/*	ListElement *head = NULL;
	int i;
	for(i = 0; i < n; i++){
		head = lruhead;
		while(head != NULL){
			Inode *inodenum = (Inode*) head->item;
			if(inodenum->inum == inodes[i]){				
				//removeThis(&lruhead, &lrutail, head);
				inodenum->inum = -1;
				inodenum = (Inode*)malloc(sizeof(Inode));
				inodenum->inum = inodes[i];
				Append(&lruhead, &lrutail, inodenum);
				break;
			}			
			head = head->next;
		}
	}*/
}

int isprobe(double probe){
	if(drand48() < probe){
		return 1;
	}
	else{
		return 0;
	}
}
int removefromcache(int inode){
	/*ListElement *head = lruhead;
	while(head != NULL){
		Inode *inodenum = (Inode*) head->item;
		if(inodenum->inum == inode){
			//removeThis(&lruhead, &lrutail, head);
			inodenum->inum = -1;
			return 1;
		}			
		head = head->next;
	}*/
	return 0;
}
