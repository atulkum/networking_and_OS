/* halt.c
 *	Simple program to test whether running a user program works.
 *
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"
void sort1(){
	int A[128];
	 int i, j, tmp;

    /* first initialize the array, in reverse sorted order */
    for (i = 0; i < 128; i++)
        A[i] = 128 - i;

    /* then sort! */
    for (i = 0; i < 127; i++)
        for (j = i; j < (127 - i); j++)
	   if (A[j] > A[j + 1]) {	/* out of order -> need to swap ! */
	      tmp = A[j];
	      A[j] = A[j + 1];
	      A[j + 1] = tmp;
    	   }
    Exit(A[0]);		/* and then we're done -- should be 0! */
}

void sort2(){
	int A[512];
	 int i, j, tmp;

    /* first initialize the array, in reverse sorted order */
    for (i = 0; i < 512; i++)
        A[i] = 512 - i;

    /* then sort! */
    for (i = 0; i < 511; i++)
        for (j = i; j < (511 - i); j++)
	   if (A[j] > A[j + 1]) {	/* out of order -> need to swap ! */
	      tmp = A[j];
	      A[j] = A[j + 1];
	      A[j + 1] = tmp;
    	   }
    Exit(A[0]);		/* and then we're done -- should be 0! */
}

int
main(){

	Write( "test3\n", 6, ConsoleOutput );
	Fork(sort1);
	Fork(sort2);
	Exit(0);
}

