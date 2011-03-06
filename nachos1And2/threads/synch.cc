// synch.cc
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts

    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    }
    value--; 					// semaphore available,
						// consume its value

    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments
// Note -- without a correct implementation of Condition::Wait(),
// the test case in the network assignment won't work!

//----------------------------------------------------------------------
// Lock::Lock
// 	Initialize a Lock, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------

Lock::Lock(char* debugName) {
#ifdef CHANGED
	marked4Deletion = false;
	name = debugName;
	queue = new List;
	ownerThread = NULL;
#endif
}
//----------------------------------------------------------------------
// Lock::~Lock
// 	De-allocate lock, when no longer needed.  Assume no one
//	is still holding the lock!
//----------------------------------------------------------------------

Lock::~Lock() {
#ifdef CHANGED
	delete queue;
#endif
}
//----------------------------------------------------------------------
// Lock::isHeldByCurrentThread
// 	Check if the lock is hold by the current running thread.
//
//----------------------------------------------------------------------
bool Lock::isHeldByCurrentThread(){
	return (ownerThread == currentThread);
}

//----------------------------------------------------------------------
// Lock::Acquire
//	Check if the lock is free. If it is free assign the lock to
//  the current thread.else append the current thread at the end
//  of the queue and wait. It is an error if a thread other than
//  current thread calls this function.
//----------------------------------------------------------------------
void Lock::Acquire() {
#ifdef CHANGED
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	if(isHeldByCurrentThread()){
		fprintf(stderr, "%s:ERROR:Lock %s is already held by the current thread.\n", currentThread->getName(),this->name);
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	if(NULL == ownerThread){
		ownerThread = currentThread;
	}
	else{
		queue->Append((void *)currentThread);
		currentThread->Sleep();
	}
	//printf("%s:Lock %s acquired.\n", currentThread->getName(),this->name);
	(void) interrupt->SetLevel(oldLevel);
#endif
}
//----------------------------------------------------------------------
// Lock::Release
//  Release the lock and assign it to a thread in the waiting queue,
//  if one is there.It is an error if a thread other than current
//  thread calls this function.
//----------------------------------------------------------------------
void Lock::Release() {
#ifdef CHANGED
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	if(!isHeldByCurrentThread()){
		fprintf(stderr, "%s:ERROR:Lock %s is not held by the current thread.\n", currentThread->getName(),this->name);
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	if(!queue->IsEmpty()){
		Thread *nextThread = (Thread *)queue->Remove();
		scheduler->ReadyToRun(nextThread);
		ownerThread = nextThread;
	}
	else{
		ownerThread = NULL;
	}
	//printf("%s:Lock %s released.\n", currentThread->getName(),this->name);
	(void) interrupt->SetLevel(oldLevel);
#endif
}

#ifdef CHANGED
//----------------------------------------------------------------------
// Lock::printWaiting
//  Function to assist debugging. Prints all the threads which are waiting
//  on this lock.
//----------------------------------------------------------------------
void Lock::printWaiting(){
	//IntStatus oldLevel = interrupt->SetLevel(IntOff);
	if(queue->IsEmpty()){
		printf("%s : EMPTY\n", this->getName());
		return;
	}
	ListElement* qf = queue->getFirst();
	ListElement* itr = qf;
	printf("%s :", this->getName());
	Thread *nextThread;
	while(itr != NULL){
		nextThread = (Thread *)itr->item;
		printf(" %s ", nextThread->getName());
		itr = itr->next;
	}
	printf("\n");
	//(void) interrupt->SetLevel(oldLevel);
}
bool Lock::isWaiting(){
	return !queue->IsEmpty();
}
#endif
//----------------------------------------------------------------------
// Condition::Condition
// 	Initialize a Condition, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------
Condition::Condition(char* debugName) {
#ifdef CHANGED
	marked4Deletion = false;
	name = debugName;
	conditionLock = NULL;
	queue = new List;
#endif
}
//----------------------------------------------------------------------
// Condition::~Condition
// 	De-allocate lock, when no longer needed.  Assume no one
//	is still signal or wait on this condition!
//----------------------------------------------------------------------
Condition::~Condition() {
#ifdef CHANGED
	delete queue;
#endif
}
//----------------------------------------------------------------------
// Condition::Wait
// 	Release the lock and Wait on the condition until some other thread
//	do a signal or broadcast, then wake up.
//  This operation must be atomic, so we need to disable interrupts
//  before checking and releasing the lock.It's an error if the calling
//  thread is not the current thread or the lock passed is not acquired
//  by the calling thread.
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------
void Condition::Wait(Lock* lock) {
#ifdef CHANGED
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	if(conditionLock == NULL){
		conditionLock = lock;
	}
	else if(conditionLock != lock){
		fprintf(stderr, "%s:ERROR:Condition %s doesn't hold the lock %s.\n",currentThread->getName(), this->name, lock->getName());
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	if(!lock->isHeldByCurrentThread()){
		fprintf(stderr, "%s:ERROR:Lock %s is not held by the current thread.\n", currentThread->getName(),lock->getName());
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	//printf("%s:Wait %s with lock %s.\n", currentThread->getName(),this->name, lock->getName());
	lock->Release();
	queue->Append((void *)currentThread);
	currentThread->Sleep();

	lock->Acquire();
	(void) interrupt->SetLevel(oldLevel);
#endif
}
//----------------------------------------------------------------------
// Condition::Signal
// 	Release a single thread which are waiting on this caonditon and put
//	it to ready queue.It's an error if the calling thread is not the
//  current thread or the lock passed is not acquired by the calling thread.
//----------------------------------------------------------------------
void Condition::Signal(Lock* lock) {
#ifdef CHANGED
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	if(queue->IsEmpty()){
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	if(conditionLock != lock){
		fprintf(stderr, "%s:ERROR:Condition %s doesn't hold the lock %s.\n",currentThread->getName(), this->name, lock->getName());
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	if(!lock->isHeldByCurrentThread()){
		fprintf(stderr, "%s:ERROR:Lock %s is not held by the current thread.\n", currentThread->getName(),lock->getName());
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	//printf("%s:Signal %s with lock %s.\n", currentThread->getName(),this->name, lock->getName());
	Thread *nextThread = (Thread *)queue->Remove();
	scheduler->ReadyToRun(nextThread);
	if(queue->IsEmpty()){
		conditionLock = NULL;
	}
	(void) interrupt->SetLevel(oldLevel);
#endif
}
//----------------------------------------------------------------------
// Condition::Broadcast
// 	Release a all the threads which are waiting on this caonditon and put
//	it to ready queue.
//----------------------------------------------------------------------
void Condition::Broadcast(Lock* conditionLockVal) {
#ifdef CHANGED
	while(!queue->IsEmpty()){
		Signal(conditionLockVal);
	}
#endif
}

#ifdef CHANGED
//----------------------------------------------------------------------
// Condition::printWaiting
//  Function to assist debugging. Prints all the threads which are waiting
//  on this condition.
//----------------------------------------------------------------------
void Condition::printWaiting(){
	//IntStatus oldLevel = interrupt->SetLevel(IntOff);
	if(queue->IsEmpty()){
		printf("%s : EMPTY\n", this->getName());
		return;
	}
	ListElement* qf = queue->getFirst();
	ListElement* itr = qf;
	printf("%s :", this->getName());
	Thread *nextThread;
	while(itr != NULL){
		nextThread = (Thread *)itr->item;
		printf(" %s ", nextThread->getName());
		itr = itr->next;
	}
	printf("\n");
	//(void) interrupt->SetLevel(oldLevel);
}

bool Condition::isWaiting(){
	return !queue->IsEmpty();
}
#endif