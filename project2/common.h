#ifndef PROJECT2_551_COMMON_H
#define PROJECT2_551_COMMON_H

#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define round551(X) (((X)>= 0)?(int)((X)+0.5):(int)((X)-0.5))
//intialize the random number generator
extern void InitRandom(long l_seed);
//get the time interval
extern int GetInterval(int exponential, double rate);
//get the time interval in exponential case
extern int ExponentialInterval(double dval, double rate);
//trim the starting and trailing blanks from astring
extern int TrimBlanks(char *str);
//return inter arrival time and service time as read from the trace
extern void readArrSer(char *str, int *a, int *s);

#endif //PROJECT2_551_COMMON_H
