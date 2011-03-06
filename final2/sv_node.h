#ifndef FINAL1_551_SV_NODE_H
#define FINAL1_551_SV_NODE_H

#include "common.h"
#include "hello.h"
#include "keepalive.h"
#include "status.h"
#include "notify.h"
#include "files.h"
#include "store.h"
#include "searchget.h"
#include "delete.h"

void* writeHandler(void *ngbrNode);
void* readHandler(void *ngbrNode);
void* readHandlerServer(void *sockid);
void* writeHandlerServer(void *ngbrNode);
void handleReadMessage(Node* ngb);
void handleWriteMessage(Node* ngb);
void *serverThreadHandler(void *dummy);
void* inputHandler(void * dummy);
void init();
void cleanup();
void softcleanup();
void closeCon(Node* nei);
void closeConWrite(Node* nei);
void reducekeepalive();
void* timerHandler(void* dummy);
#endif //FINAL1_551_SV_NODE_H
