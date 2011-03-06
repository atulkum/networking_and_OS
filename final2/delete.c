#include "delete.h"

void processDelete(char *filename, char *sha1, char *nonce){
	char password[41];
	int *inodes;
	int nfile;
	int done = 0;
	Metadata mdata;
	if(searchSha1(sha1, &inodes, &nfile) == 0){
		struct stat stFileInfo;
		int i;
		char passfile[256];
		for(i = 0; i < nfile; ++i){								
			readMetadata(&mdata, inodes[i]);
			//printf("inode %d mdata %s %s\n", inodes[i], mdata.nonce, nonce);
			//printf("inode %d mdata %s %s\n", inodes[i], mdata.filename, filename);
			if(!strcmp(mdata.nonce, nonce) && !strcmp(mdata.filename, filename)){														
				memset(passfile, 0, 256);
				sprintf(passfile, "%s/files/%d.pass", homeDir, inodes[i]);				
				if(stat(passfile, &stFileInfo) == 0){
					memset(password, 0, 41);
					FILE* pass = fopen (passfile, "r");
					if(pass == NULL){
						fprintf(logfile, "** ERROR: file %s open fails\n", passfile);fflush(logfile);
					}			
					fgets(password, 41, pass);		
					fclose(pass);
					password[40] = '\0';
					//printf("password %s\n", password);
					done = 1;
					break;
				}
			}
		}
	}
	if(done == 0){
		printf ("\nNo one-time password found.\nOkay to use a random password [yes/no]?");			
		char string[256];
		memset(string, 0, 256);
		char* temp = gets(string);
		if(temp != NULL && (string[0] == 'y' || string[0] == 'Y')){
			char uoid[20];
			memset(uoid, 0, 20);
			GetUOID(filename, "password", uoid, 20);				
			memset(password, 0, 41);
			int2hex((unsigned char*)uoid, 20, password);				
		}	
		else{
			return;
		}
	}
	else{
		deleteFiles(&mdata);
	}
	sendDelete(filename, sha1, nonce, password);
}

void sendDelete(char *fname, char* s1, char* nce, char *pwd){
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
			Message * msg = (Message*)malloc(sizeof(Message));
			if (msg == NULL) {
				fprintf(logfile, "** malloc() failed\n");fflush(logfile);
			}
			prepareDelete(msg, uoid, fname, s1, nce, pwd);						
			writeLog(SEND, nei->nodeId, msg->dataLength, msg->ttl, DELT, "", msg->uoid);
		
			pthread_mutex_lock(&(nei->nodeLock));				
			Append(&(nei->writeQhead), &(nei->writeQtail), msg);
			nei->qlen++;
			pthread_mutex_unlock(&(nei->nodeLock));
			pthread_cond_signal(&(nei->nodeCond));			
		}
		head = head->next;
	}
}


void prepareDelete(Message *msg, char* uoid, char *fname, char* s1, char* nce, char *pwd){
	memset(msg, 0, sizeof(Message));
	msg->type = DELETE;
	memset(msg->uoid, 0, 20);
	memcpy(msg->uoid, uoid, 20);
	msg->ttl = ttl;	
	msg->dataLength = 40 + 40 + 40 + strlen(fname);	
	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}	
	int off = 0;
	memcpy(msg->data + off, s1, 40); off += 40;
	memcpy(msg->data + off, nce, 40); off += 40;
	memcpy(msg->data + off, pwd, 40); off += 40;
	memcpy(msg->data + off, fname, 40); off += strlen(fname);
}
void deleteAction(int len, unsigned char* buf){
	char sha1[41];
	char nonce[41];
	char passwd[41];
	char name[256];

	memset(sha1, 0, 41); memcpy(sha1, buf, 40);sha1[40] = '\0';
	memset(nonce, 0, 41); memcpy(nonce, buf + 40, 40);nonce[40] = '\0';
	memset(passwd, 0, 41); memcpy(passwd, buf + 80, 40);passwd[40] = '\0';
	memset(name, 0, 41); memcpy(name, buf + 120, len - 120);
	name[len - 120] = '\0';

	int *inodes;
	int nfile;
	char uoid[20];
	char gennonce[41];
	Metadata mdata;
	memset(uoid, 0, 20);
	SHA1((unsigned char*)passwd, 40, (unsigned char*)uoid);						
	memset(gennonce, 0, 41);
	int2hex((unsigned char*)uoid, 20, gennonce);
	gennonce[40] = '\0';

	if(!strcmp(gennonce, nonce)){
		if(searchSha1(sha1, &inodes, &nfile) == 0){
				printf("sha1 %s \n",sha1);
			int i;
			for(i = 0; i < nfile; ++i){					
				readMetadata(&mdata, inodes[i]);			
				if(!strcmp(mdata.nonce, nonce) && !strcmp(mdata.filename, name)){												
					deleteFiles(&mdata);
					break;
				}
			}			
		}
	}
}

