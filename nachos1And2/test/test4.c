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
int lockid, initlock;
int condid, initcond;
int i = 0;
void waiter() {
	Acquire(initlock);
	Signal(initcond, initlock);
	Release(initlock);

	Write( "waiting\n", 8, ConsoleOutput );
	Acquire(lockid);
	Wait(condid, lockid);
	Write( "waiter wakeup\n", 14, ConsoleOutput );
	Release(lockid);

	Acquire(initlock);
	i++;
	Signal(initcond, initlock);
	Release(initlock);

    Exit(0);
}
void signaller() {
	Write( "Signaling\n", 10, ConsoleOutput );
	Acquire(lockid);
	Signal(condid, lockid);
	Write( "signaller wakeup\n", 17, ConsoleOutput );
	Release(lockid);

	Acquire(initlock);
	i++;
	Signal(initcond, initlock);
	Release(initlock);

    Exit(0);
}

int
main(){
	lockid = CreateLock("LOCK1", 5);
	initlock = CreateLock("LOCK2", 5);
	condid = CreateCondition("COND1", 5);
	initcond = CreateCondition("COND2", 5);

	Write( "test4\n", 6, ConsoleOutput );
	Acquire(initlock);
	Fork(waiter);
	Wait(initcond, initlock);

	Fork(signaller);
	while(i != 2){
		Wait(initcond, initlock);
	}
	Release(initlock);

	DestroyLock(lockid);
	DestroyLock(initlock);
	DestroyCondition(condid);
	DestroyCondition(initcond);
	Exit(0);
}

