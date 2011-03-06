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
int i1;
int i2;
int i3;
int i4;
int i5;
void waiter() {
	if(i1 == 2){
		Write("i1w2\n",5,ConsoleOutput);
	}
	if(i1 == 0){
		Write("i1w0\n",5,ConsoleOutput);
	}
	if(i2 == 2){
		Write("i2w2\n",5,ConsoleOutput);
	}
	if(i2 == 0){
		Write("i2w0\n",5,ConsoleOutput);
	}
	if(i3 == 2){
		Write("i3w2\n",5,ConsoleOutput);
	}
	if(i3 == 0){
		Write("i3w0\n",5,ConsoleOutput);
	}
	if(i4 == 2){
		Write("i4w2\n",5,ConsoleOutput);
	}
	if(i4 == 0){
		Write("i4w0\n",5,ConsoleOutput);
	}
	if(i5 == 2){
		Write("i5w2\n",5,ConsoleOutput);
	}
	if(i5 == 0){
		Write("i5w0\n",5,ConsoleOutput);
	}

    Exit(0);
}
void signaller() {
	if(i1 == 2){
		Write("i1s2\n",5,ConsoleOutput);
	}
	if(i1 == 0){
		Write("i1s0\n",5,ConsoleOutput);
	}
	if(i2 == 2){
		Write("i2s2\n",5,ConsoleOutput);
	}
	if(i2 == 0){
		Write("i2s0\n",5,ConsoleOutput);
	}
	if(i3 == 2){
		Write("i3s2\n",5,ConsoleOutput);
	}
	if(i3 == 0){
		Write("i3s0\n",5,ConsoleOutput);
	}
	if(i4 == 2){
		Write("i4s2\n",5,ConsoleOutput);
	}
	if(i4 == 0){
		Write("i4s0\n",5,ConsoleOutput);
	}
	if(i5 == 2){
		Write("i5s2\n",5,ConsoleOutput);
	}
	if(i5 == 0){
		Write("i5s0\n",5,ConsoleOutput);
	}

    Exit(0);
}
int
main(){
	i1 = 2;
	i2 = 2;
	i3 = 2;
	i4 = 2;
	i5 = 2;

	if(i1 == 2){
		Write("i1 2\n",5,ConsoleOutput);
	}
	if(i1 == 0){
		Write("i1 0\n",5,ConsoleOutput);
	}
	if(i2 == 2){
		Write("i2 2\n",5,ConsoleOutput);
	}
	if(i2 == 0){
		Write("i2 0\n",5,ConsoleOutput);
	}
	if(i3 == 2){
		Write("i3 2\n",5,ConsoleOutput);
	}
	if(i3 == 0){
		Write("i3 0\n",5,ConsoleOutput);
	}
	if(i4 == 2){
		Write("i4 2\n",5,ConsoleOutput);
	}
	if(i4 == 0){
		Write("i4 0\n",5,ConsoleOutput);
	}
	if(i5 == 2){
		Write("i5 2\n",5,ConsoleOutput);
	}
	if(i5 == 0){
		Write("i5 0\n",5,ConsoleOutput);
	}
	Fork(waiter);
	Fork(signaller);
	Exit(0);
}

