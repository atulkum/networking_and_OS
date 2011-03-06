#ifndef FINAL1_551_COMMON_H
#define FINAL1_551_COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <openssl/sha.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/time.h>

#include "list.h"

#ifndef min
#define min(A,B) (((A)>(B)) ? (B) : (A))
#endif /* ~min */

#define MAX_LINE_SIZE 1024
#define SHA_DIGEST_LENGTH 20

#define KW_PORT 1 
#define KW_LOCATION 2 
#define KW_HOMEDIR 3
#define KW_LOGFILE 4
#define KW_AUTOSHUTDOWN 5
#define KW_TTL 6
#define KW_MSGLIFETIME 7
#define KW_GETMSGLIFETIME 8
#define KW_INITNGBR 9
#define KW_JOINTIMEOUT 10
#define KW_KEEPALIVE 11 
#define KW_MINNGBR 12
#define KW_NOCHECK 13 
#define KW_CACHEPROB 14
#define KW_STOREPROB 15
#define KW_NGBRSTORPROB 16
#define KW_CACHESIZE 17
#define KW_PERMSIZE 18
#define KW_RETRY 19
#define INVALID -1
 
#define JNRQ 1
#define JNRS 2
#define HLLO 3
#define KPAV 4
#define NTFY 5
#define CKRQ 6
#define CKRS 7
#define SHRQ 8
#define SHRS 9
#define GTRQ 10
#define GTRS 11
#define STOR 12
#define DELT 13
#define STRQ 14
#define STRS 15

#define RCV 0
#define FORW 1
#define SEND 2
#define ERR  3
#define DEB 4

typedef struct tagKwInfo {
	int id;
	char *key;
	int mandatory; 
} KwInfo;
extern KwInfo gkwinfo[];

#define STATUS_TYPE_NEIGHBORS 0x01
#define STATUS_TYPE_FILES 0x02

#define HELLO 0xFA
#define JOIN 0xFC
#define JOINRESPONSE 0xFB
#define NOTIFY 0xF7
#define KEEPALIVE 0xF8
#define STATUS 0xAC
#define STATUSRESPONSE 0xAB
#define CHECK 0xF6
#define CHECKRESPONSE 0xF5
#define NOTIFY 0xF7

#define UNKNOWN 0
#define USER_SHUTDOWN 1
#define UNEXPECTED_KILL 2
#define SELF_RESTART 3

typedef struct message{
	uint8_t type;
	int8_t uoid[20];
	uint8_t ttl;
	uint32_t dataLength;
	unsigned char *data;
}Message; 

typedef struct node{
	char nodeId[512];
	uint16_t port;
	char hostname[512];	
	
	int qlen;
	pthread_cond_t nodeCond;
	pthread_mutex_t nodeLock;
	
	pthread_t readThread;
	pthread_t writeThread;

	ListElement* writeQhead;
	ListElement* writeQtail;
	int sock_id;
	int isHelloDone;
	int terminate;
	uint16_t kplvLifeTime;
	int istemp;
}Node;

typedef struct routerNode{
	int8_t uoidkey[20];
	uint16_t lifetime;
	//char nodeId[512];
	Node* tosend;
}RouterNode;

char *GetUOID(char *node_inst_id, char *obj_type, char *uoid_buf, int uoid_buf_sz);
void printhex(unsigned char *src ,int n);
void int2hex(unsigned char *src ,int n, char*toprint);
int TrimBlanks(char *str);
int GetKeyValue(char *buf, char separator, char **ppsz_key, char **ppsz_value);
int writeByte(int fd, unsigned char* vptr, int n);
int readByte(int fd, unsigned char* vptr, int n );
void getNodeInstId(char* id_buf, char* instName);
void getNodeId(char* name_buf, char* hostname, uint16_t portno);
void sigpipe_handler(int sig_num);
void sigusr1_handler(int sig_num);
char* getIPAddr(char* host);
int sendMessage(int sock_id, Message* msg, char* sentid);
RouterNode* findRouterEntry(int8_t* uoid);
void reduceLifeTime();
void writeLog(int dir, char* nodeid, int size, int ttl, int type, char* data, int8_t* msguoid);
void writingSameResMsg( Message* msg, unsigned char* data, Node* ngbrtosend);
void writingSameResMsgttldec( Message* msg, unsigned char* data, Node* ngbrtosend);
int iniparser(char * filename);
int ngbrParser();
void processvalue(int id, char * value);
void readMessage(Message* msg, unsigned char* data);
Node* createNode(char* hostname, uint16_t portno);
void putNewRouteEntry(int8_t* uoid, Node* ngb);
void printlogHello(Message* msg, unsigned char* data, char* nodeid, int issend);
void printJoin(Message *msg, unsigned char* data, char* nodeid, int issend);
void printJoinResponse(Message *msg, unsigned char* data, char* nodeid, int issend);
Node* findNode(Node* nei, ListElement* head);
Node* createEmptyNode();
void destroyNode(Node* n);
void sigalrm_handler(int sig_num);

extern time_t starttime;
extern char logfileName[512];
extern uint16_t autoShutdown;
extern uint16_t ttl;
extern uint16_t msgLifeTime;
extern uint16_t getMsgLifeTime;
extern unsigned char initNgbr;// beacon node ignore it
extern uint16_t joinTimeout; //beacon node ignore it
extern uint16_t keepalive;
extern unsigned char minNgbr;// beacon node ignore it
extern unsigned char noCheck; 
extern double cacheProb;
extern double storeProb;
extern double ngbrStoreProb; 
extern uint16_t cacheSize;
extern uint16_t permSize;//obsolete
extern uint16_t retry;

extern uint16_t myport; 
extern uint32_t location;
extern char homeDir[512]; // dir name 512 byte, write in readme
extern char myhostname[256];
extern int isBeacon;

extern ListElement* beaconshead;
extern ListElement* beaconstail;

extern ListElement* negbhead;
extern ListElement* negbtail;

extern ListElement* conhead;
extern ListElement* contail;

extern char mynodeId[512];

extern ListElement* routerhead;
extern ListElement* routertail;
extern pthread_mutex_t routerLock;
extern FILE* logfile;
extern pthread_mutex_t printLock;

//all threads
extern pthread_t serverThread;
extern pthread_t keepaliveThread;
extern pthread_t inputThread;
extern pthread_t statusThread;
extern pthread_t joinThread;
extern pthread_t checkThread;
extern pthread_t timerThread;

///////////////////////////////////////////
extern int isStatusOn;
extern pthread_t statusThread;
typedef struct statusNode{
	char nodeId[512];
	char neibr[10][512]; //assuming 10 neighbour
	int nnei;
}StatusNode;
extern int8_t statusuoid[20];
extern StatusNode statusDB[10];
extern int statusDBlen;
extern char statusFilename[512];
extern pthread_mutex_t statusDBLock;
///////////////////////////////////////////
extern pthread_t joinThread;
typedef struct joinNode{
	char host[512];
	uint16_t port;
	uint32_t distance;
	int isdone;
}JoinNode;
extern int8_t joinuoid[20];
extern JoinNode joinDB[10];
extern int joinDBlen;
extern pthread_mutex_t joinDBLock;
///////////////////////////////////////////
extern char initNgbrPath[512];
extern char logfilePath[512];
extern int reset;
extern int8_t checkuoid[20];
extern int isCheckOk;
extern int isJoinDone;
extern int isShutdown;
extern int softshutdown;
#endif //FINAL1_551_COMMON_H
