#ifndef FINAL1_551_HELLO_H
#define FINAL1_551_HELLO_H
#include "common.h"
int processHello(Message* msg, unsigned char* temp, Node* ngbr);
void prepareHello(Message *msg);
int sendHelloBeacon(Node* nei);
int sendHelloNonBeacon(Node* nei);

#endif //FINAL1_551_HELLO_H
