#ifndef PROJECT2_551_MM2_H
#define PROJECT2_551_MM2_H

#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "common.h"
#include "list.h"
//type of distribution
#define DISTR_EXP 1
#define DISTR_DET 0

//parameters
extern double lambda; // lambda value
extern double mu; // mu value
extern int isSingleServer;//0 = double, 1 = single
extern long seedval;// seed value
extern unsigned int qsize; // Q1 size
extern unsigned int nCustomer; //number of customer
extern int distr; // 0 = exp, 1 = det
extern int isTraceFile; //0 = no trace file, 1 = use trace file
	//(-lambda, -mu, -seed, -n, and -d don't print these)
//calculate time elapsed between tv2 and tv1 (tv2 - tv1 = diff)
extern void timeElapsed(struct timeval* tv1, struct timeval* tv2, struct timeval* diff);
//thread handler for thread
extern void* serverThreadHandler(void *serverid);
//initialization function
extern int init();
//cleanup function
extern void destroy();
//print initialization message
extern void printStartMsg();
//print a message with time stamp
extern void printtimemsg(struct timeval* tv, char* msg);
//interrupt handler thread method
extern void* interruptThreadHandler(void *data);
//interrupt handler
extern void interrupt(int sig);
//no op interrupt handler
extern void dummyInterrupt(int sig);
//it parse the command line
extern int ProcessOptions(int argc, char **argv);
//Customer data type
typedef struct customer{
	struct timeval a; //arrival time - filled by arriaval thread
	struct timeval qs; // queuing time - filled by arrival thread
	struct timeval qf; // dequeuing time - filled by server
	struct timeval s; //service start time - filled by server
	struct timeval d; //departure time - filled by server
	int id;
	int sertime;
}Customer;

#endif //PROJECT2_551_MM2_H
