#ifndef FINAL1_551_DELETE_H
#define FINAL1_551_DELETE_H
#include "common.h"
#include "files.h"
void processDelete(char *filename, char *sha1, char *nonce);
void sendDelete(char *fname, char* s1, char* nce, char *pwd);
void prepareDelete(Message *msg, char* uoid, char *fname, char* s1, char* nce, char *pwd);
void deleteAction(int len, unsigned char* buf);
#endif //FINAL1_551_DELETE_H
