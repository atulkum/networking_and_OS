#include "notify.h"
/*
This method would build the NOTIFY message.
*/
void prepareNotify(Message *msg){
	memset(msg, 0, sizeof(Message));
	char inst_id[512];	
	memset(inst_id, 0, 512);	
	msg->type = NOTIFY;
	memset(msg->uoid, 0, 20);
	getNodeInstId(inst_id, mynodeId);
	GetUOID(inst_id, "msg", msg->uoid, 20);
	msg->ttl = 1;	
	msg->dataLength = 1;
	msg->data = (unsigned char*)malloc(msg->dataLength);
	if (msg->data == NULL) {
		fprintf(logfile, "** malloc() failed\n");fflush(logfile);
	}
	uint8_t type = USER_SHUTDOWN;

	memcpy(msg->data, &(type), msg->dataLength); 
}
