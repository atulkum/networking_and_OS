#ifndef FINAL1_551_SEARCHGET_H
#define FINAL1_551_SEARCHGET_H
#include "common.h"
#include "files.h"
void processSearch(char* line);
void prepareSearch(Message *msg, char* uoid, uint8_t type, char* value);
void sendSearchResponse(char* uoid, Node* ngb, uint8_t searchtype, char* value);
void processSearchResponse(int len, unsigned char* data);

void processGet(char* filename, int id);
void writeAfterGet(int inode);
void prepareGet(Message *msg, char* uoid, char *fileid);
void sendGetResponse(char *uoid, Node* ngb, int inode);
int sendGet(Node* ngb, char* uoid, char* buffer, int length, int inode, int filesize, int issend);
#endif //FINAL1_551_SEARCHGET_H


