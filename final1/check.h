#ifndef FINAL1_551_CHECK_H
#define FINAL1_551_CHECK_H
#include "common.h"

void prepareCheck(Message *msg, int8_t* uoid);
void prepareCheckResponse(Message *msg, int8_t* uoid);
void* checkHandler(void *dummy);
void doCheck();
void sendCheckResponse(int8_t* uoid, Node* ngb);
#endif //FINAL1_551_CHECK_H
