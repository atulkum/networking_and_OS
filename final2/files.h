#ifndef FINAL1_551_FILES_H
#define FINAL1_551_FILES_H
#include "common.h"

typedef struct kwrd_index{
	int inodeno[16];
	int n;
	unsigned char bitvecSHA1[64];
	unsigned char bitvecMD5[64];
}kwrdIndex;

typedef struct name_index{
	int inodeno[16];
	int n;
	char filename[256];
}nameIndex;

typedef struct sha1_index{
	int inodeno[16];
	int n;
	char sha1[40 + 1];
}sha1Index;

typedef struct metadata{
	int inodeno;
	char filename[256];
	uint32_t filesize;
	char sha1[40 + 1];
	char nonce[40 + 1];
	int nkeys;
	char keywords[10][64];
	unsigned char bitvecSHA1[64];
	unsigned char bitvecMD5[64];
}Metadata;

typedef struct inode{
	int inum;
}Inode;

typedef struct metainfo{
	char buffer[512];
	uint32_t reclen;
}Metainfo;

typedef struct searchinfo{
	char buffer[512];
	uint32_t reclen;
	char uoid[20];
}Searchinfo;

typedef struct inodeUoid{
	char uoid[20];
	int inode;
}InodeUoid;

int getNextFileInode();
int writeMetadata(char *buffer, int inode);
int prepareMetadata(Metadata *mdata, char *storedfilename);
int readMetadata(Metadata *mdata, int inum);
int readMetadataFromFile4Search(char* buffer, int inum);
void tolowerCase(char* str, char* lowered);
void setBitVectors(unsigned char *sha1, unsigned char *md5, char *key);
int dumpMetadata(Metadata *mdata, char *buffer);
void updateIndexFiles(Metadata *mdata, int iscache);
int dumpIndexFiles();
int readMetadataFromFile(char* buffer, int inum);
void filecleanup();
int searchBitVector(char* key, int **inodes, int* len);
int searchName(char* key, int **inodes, int* len);
int searchSha1(char* key, int **inodes, int* len);
InodeUoid* getFileId(int inode);
unsigned char isSet(unsigned char *sha1, unsigned char *md5, char *key);
int isFileIdExists(char *fileid);
int reloadIndexFiles();
void deleteFiles(Metadata *mdata);
void updateLRUonSearch(int* inodes, int n);
int isprobe(double probe);
int removefromcache(int inode);

#endif //FINAL1_551_FILES_H
