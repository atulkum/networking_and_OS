#ifndef FINAL1_551_STORE_H
#define FINAL1_551_STORE_H
#include "common.h"
#include "files.h"

int sendStore(Node* ngb, uint8_t ttl, char* uoid, char* buffer, int length, int inode, int filesize, int issend);
void doStore(uint16_t ttltosend, char* filename);
int readMetadataAndStoreFile(int sock_id, int datalen, uint32_t* metadataLen, unsigned char* buffer);

#endif //FINAL1_551_STORE_H
