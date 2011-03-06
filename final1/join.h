#ifndef FINAL1_551_JOIN_H
#define FINAL1_551_JOIN_H

#include "common.h"

void* joinHandler(void *dummy);
void prepareJoin(Message *msg, int8_t* uoid);
void prepareJoinResponse(Message *msg, int8_t* uoid, uint32_t rcvloc);
void storeJoinInfoNgbr(int len, unsigned char* data);
void processJoin(Message* msg, unsigned char* temp,  Node* ngbr);
void handleJoinRead(Node* other, Message* msg, unsigned char* data);
int dojoin();
#endif //FINAL1_551_STATUS_H
