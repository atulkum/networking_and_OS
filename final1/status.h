#ifndef FINAL1_551_STATUS_H
#define FINAL1_551_STATUS_H
#include "common.h"

void* statusHandler(void *dummy);
void prepareStatusWithUoid(Message *msg, uint8_t ttl, int isNgbr, int8_t* uoid);
void sendStatusResponse(int isNgbr, uint32_t length, int8_t* uoid, Node* ngb);
void storeStatusInfoNgbr(int len, unsigned char* data);
void interrupt(int sig);
#endif //FINAL1_551_STATUS_H
