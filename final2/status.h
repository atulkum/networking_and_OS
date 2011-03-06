#ifndef FINAL1_551_STATUS_H
#define FINAL1_551_STATUS_H
#include "common.h"
#include "files.h"

typedef struct statusNode{
	uint16_t thisport;
	uint16_t neibr[10]; //assuming 10 neighbour
	int nnei;
}StatusNode;

void statusHandler();
void prepareStatus(Message *msg, uint8_t ttl, char* uoid, uint8_t type);
void sendStatusResponse(char* uoid, Node* ngb);
void storeStatusInfo(int len, unsigned char* data);
void interrupt(int sig);
void doStatusNgbr(uint16_t ttltosend);
void doStatusFile(uint16_t ttltosend);
void sendStatusFileResponse( char* uoid, Node* ngb);
#endif //FINAL1_551_STATUS_H
