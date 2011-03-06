// threadtest.cc
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield,
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#ifdef CHANGED
#include "synch.h"
#endif
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;

    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
    DEBUG('t', "Entering SimpleTest");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

#ifdef CHANGED
// --------------------------------------------------
// Test Suite
// --------------------------------------------------


// --------------------------------------------------
// Test 1 - see TestSuite() for details
// --------------------------------------------------
Semaphore t1_s1("t1_s1",0);       // To make sure t1_t1 acquires the
                                  // lock before t1_t2
Semaphore t1_s2("t1_s2",0);       // To make sure t1_t2 Is waiting on the
                                  // lock before t1_t3 releases it
Semaphore t1_s3("t1_s3",0);       // To make sure t1_t1 does not release the
                                  // lock before t1_t3 tries to acquire it
Semaphore t1_done("t1_done",0);   // So that TestSuite knows when Test 1 is
                                  // done
Lock t1_l1("t1_l1");		  // the lock tested in Test 1

// --------------------------------------------------
// t1_t1() -- test1 thread 1
//     This is the rightful lock owner
// --------------------------------------------------
void t1_t1() {
    t1_l1.Acquire();
    t1_s1.V();  // Allow t1_t2 to try to Acquire Lock

    printf ("%s: Acquired Lock %s, waiting for t3\n",currentThread->getName(),
	    t1_l1.getName());
    t1_s3.P();
    printf ("%s: working in CS\n",currentThread->getName());
    for (int i = 0; i < 1000000; i++) ;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t2() -- test1 thread 2
//     This thread will wait on the held lock.
// --------------------------------------------------
void t1_t2() {

    t1_s1.P();	// Wait until t1 has the lock
    t1_s2.V();  // Let t3 try to acquire the lock

    printf("%s: trying to acquire lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Acquire();

    printf ("%s: Acquired Lock %s, working in CS\n",currentThread->getName(),
	    t1_l1.getName());
    for (int i = 0; i < 10; i++)
	;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t3() -- test1 thread 3
//     This thread will try to release the lock illegally
// --------------------------------------------------
void t1_t3() {

    t1_s2.P();	// Wait until t2 is ready to try to acquire the lock

    t1_s3.V();	// Let t1 do it's stuff
    for ( int i = 0; i < 3; i++ ) {
	printf("%s: Trying to release Lock %s\n",currentThread->getName(),
	       t1_l1.getName());
	t1_l1.Release();
    }
}

// --------------------------------------------------
// Test 2 - see TestSuite() for details
// --------------------------------------------------
Lock t2_l1("t2_l1");		// For mutual exclusion
Condition t2_c1("t2_c1");	// The condition variable to test
Semaphore t2_s1("t2_s1",0);	// To ensure the Signal comes before the wait
Semaphore t2_done("t2_done",0);     // So that TestSuite knows when Test 2 is
                                  // done

// --------------------------------------------------
// t2_t1() -- test 2 thread 1
//     This thread will signal a variable with nothing waiting
// --------------------------------------------------
void t2_t1() {
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t2_l1.getName(), t2_c1.getName());
    t2_c1.Signal(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t2_l1.getName());
    t2_l1.Release();
    t2_s1.V();	// release t2_t2
    t2_done.V();
}

// --------------------------------------------------
// t2_t2() -- test 2 thread 2
//     This thread will wait on a pre-signalled variable
// --------------------------------------------------
void t2_t2() {
    t2_s1.P();	// Wait for t2_t1 to be done with the lock
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t2_l1.getName(), t2_c1.getName());
    t2_c1.Wait(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t2_l1.getName());
    t2_l1.Release();
}
// --------------------------------------------------
// Test 3 - see TestSuite() for details
// --------------------------------------------------
Lock t3_l1("t3_l1");		// For mutual exclusion
Condition t3_c1("t3_c1");	// The condition variable to test
Semaphore t3_s1("t3_s1",0);	// To ensure the Signal comes before the wait
Semaphore t3_done("t3_done",0); // So that TestSuite knows when Test 3 is
                                // done

// --------------------------------------------------
// t3_waiter()
//     These threads will wait on the t3_c1 condition variable.  Only
//     one t3_waiter will be released
// --------------------------------------------------
void t3_waiter() {
    t3_l1.Acquire();
    t3_s1.V();		// Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t3_l1.getName(), t3_c1.getName());
    t3_c1.Wait(&t3_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t3_c1.getName());
    t3_l1.Release();
    t3_done.V();
}


// --------------------------------------------------
// t3_signaller()
//     This threads will signal the t3_c1 condition variable.  Only
//     one t3_signaller will be released
// --------------------------------------------------
void t3_signaller() {

    // Don't signal until someone's waiting

    for ( int i = 0; i < 5 ; i++ )
	t3_s1.P();
    t3_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t3_l1.getName(), t3_c1.getName());
    t3_c1.Signal(&t3_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t3_l1.getName());
    t3_l1.Release();
    t3_done.V();
}

// --------------------------------------------------
// Test 4 - see TestSuite() for details
// --------------------------------------------------
Lock t4_l1("t4_l1");		// For mutual exclusion
Condition t4_c1("t4_c1");	// The condition variable to test
Semaphore t4_s1("t4_s1",0);	// To ensure the Signal comes before the wait
Semaphore t4_done("t4_done",0); // So that TestSuite knows when Test 4 is
                                // done

// --------------------------------------------------
// t4_waiter()
//     These threads will wait on the t4_c1 condition variable.  All
//     t4_waiters will be released
// --------------------------------------------------
void t4_waiter() {
    t4_l1.Acquire();
    t4_s1.V();		// Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t4_l1.getName(), t4_c1.getName());
    t4_c1.Wait(&t4_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t4_c1.getName());
    t4_l1.Release();
    t4_done.V();
}


// --------------------------------------------------
// t2_signaller()
//     This thread will broadcast to the t4_c1 condition variable.
//     All t4_waiters will be released
// --------------------------------------------------
void t4_signaller() {

    // Don't broadcast until someone's waiting

    for ( int i = 0; i < 5 ; i++ )
	t4_s1.P();
    t4_l1.Acquire();
    printf("%s: Lock %s acquired, broadcasting %s\n",currentThread->getName(),
	   t4_l1.getName(), t4_c1.getName());
    t4_c1.Broadcast(&t4_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t4_l1.getName());
    t4_l1.Release();
    t4_done.V();
}
// --------------------------------------------------
// Test 5 - see TestSuite() for details
// --------------------------------------------------
Lock t5_l1("t5_l1");		// For mutual exclusion
Lock t5_l2("t5_l2");		// Second lock for the bad behavior
Condition t5_c1("t5_c1");	// The condition variable to test
Semaphore t5_s1("t5_s1",0);	// To make sure t5_t2 acquires the lock after
                                // t5_t1

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a condition under t5_l1
// --------------------------------------------------
void t5_t1() {
    t5_l1.Acquire();
    t5_s1.V();	// release t5_t2
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t5_l1.getName(), t5_c1.getName());
    t5_c1.Wait(&t5_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l1.getName());
    t5_l1.Release();
}

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a t5_c1 condition under t5_l2, which is
//     a Fatal error
// --------------------------------------------------
void t5_t2() {
    t5_s1.P();	// Wait for t5_t1 to get into the monitor
    t5_l1.Acquire();
    t5_l2.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t5_l2.getName(), t5_c1.getName());
    t5_c1.Signal(&t5_l2);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l2.getName());
    t5_l2.Release();
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l1.getName());
    t5_l1.Release();
}

// --------------------------------------------------
// TestSuite()
//     This is the main thread of the test suite.  It runs the
//     following tests:
//
//       1.  Show that a thread trying to release a lock it does not
//       hold does not work
//
//       2.  Show that Signals are not stored -- a Signal with no
//       thread waiting is ignored
//
//       3.  Show that Signal only wakes 1 thread
//
//	 4.  Show that Broadcast wakes all waiting threads
//
//       5.  Show that Signalling a thread waiting under one lock
//       while holding another is a Fatal error
//
//     Fatal errors terminate the thread in question.
// --------------------------------------------------
void TestSuite() {
    Thread *t;
    char *name;
    int i;

    // Test 1

    printf("Starting Test 1\n");

    t = new Thread("t1_t1");
    t->Fork((VoidFunctionPtr)t1_t1,0);

    t = new Thread("t1_t2");
    t->Fork((VoidFunctionPtr)t1_t2,0);

    t = new Thread("t1_t3");
    t->Fork((VoidFunctionPtr)t1_t3,0);

    // Wait for Test 1 to complete
    for (  i = 0; i < 2; i++ )
	t1_done.P();

    // Test 2

    printf("Starting Test 2.  Note that it is an error if thread t2_t2\n");
    printf("completes\n");

    t = new Thread("t2_t1");
    t->Fork((VoidFunctionPtr)t2_t1,0);

    t = new Thread("t2_t2");
    t->Fork((VoidFunctionPtr)t2_t2,0);

    // Wait for Test 2 to complete
    t2_done.P();

    // Test 3

    printf("Starting Test 3\n");

    for (  i = 0 ; i < 5 ; i++ ) {
		name = new char [20];
		sprintf(name,"t3_waiter%d",i);
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)t3_waiter,0);
    }
    t = new Thread("t3_signaller");
    t->Fork((VoidFunctionPtr)t3_signaller,0);

    // Wait for Test 3 to complete
    for (  i = 0; i < 2; i++ )
	t3_done.P();

    // Test 4

    printf("Starting Test 4\n");

    for (  i = 0 ; i < 5 ; i++ ) {
	name = new char [20];
	sprintf(name,"t4_waiter%d",i);
	t = new Thread(name);
	t->Fork((VoidFunctionPtr)t4_waiter,0);
    }
    t = new Thread("t4_signaller");
    t->Fork((VoidFunctionPtr)t4_signaller,0);

    // Wait for Test 4 to complete
    for (  i = 0; i < 6; i++ )
	t4_done.P();

    // Test 5

    printf("Starting Test 5.  Note that it is an error if thread t5_t1\n");
    printf("completes\n");

    t = new Thread("t5_t1");
    t->Fork((VoidFunctionPtr)t5_t1,0);

    t = new Thread("t5_t2");
    t->Fork((VoidFunctionPtr)t5_t2,0);

}

/*******************************Simulating an Amusement Ride***************************************/
#define MAX_CAR  13
#define LINES 4
#define SEATS_PER_LINE 2

//These are the parameter of the simulation. The user should inputs
//all the values.
//nCustomer is number of customers
//nCar is number of cars
//nHelper is number of helpers
//nRide is number of rides
//In every ride the operator first load all the cars as much as possible
//and then run all the cars and then unload the cars in the same order
//as it was loaded.
int nCustomer = 80;
int nCar = 5;
int nHelper = 4;
int nRide = 2;
//loadLock -- it is used in synchronous access to currentCar and helperEnd.
//helperEnd -- stores the count of the signals all the Helpers send to
//the operator.
//currentCar -- It is the id of the current car which is loading or unloading.
//allAboard -- Using this the helpers inform the operator
//that all the cars are loaded as much as possible, and ride
//is ready to go.
//loadHelper -- helpers wait on this condition for a new car to start
//downloading
Lock* loadLock =  new Lock("LOAD LOCK");
int currentCar = -1;
int helperEnd = 0;
Condition* allAboard = new Condition("ALL ABOARD COND");
Condition* loadHelper[MAX_CAR + 1];
//custLock -- it is used in synchronous access to custInline,seatedCount
//and remainingCust.
//custInline -- this array stores the number of customer in each line.
//seatedCount -- this array stores the number of customer seated in
//each car.
//remainingCust -- it stores the total number of remaining customers
//in the line. basically it is sum over all custInline values.
//inLine -- the customer wait in line using this signal and the
//helper signal on it when it reserve a seat. This is for every line.
//lastSafelyUnloaded -- using this condition the Customer which is
//unloaded from a car at the end, signal the operator that unloading
//is done.
//allUnboard -- using this condition the operator broadcaste all the
//customer in cars to unoload.
Lock* custLock = new Lock("CUSTOMER LOCK");
Condition* inLine[LINES];
int custInline[LINES];
int seatedCount[MAX_CAR + 1];
int remainingCust = nCustomer;
Condition* lastSafelyUnloaded = new Condition("LAST SAFELY UNLOAD COND");
Condition* allUnboard[MAX_CAR + 1];

//perLineLock -- it is used for locking the access to a particular line while
//a helper is serving that line. So that only one helper can serve a line
//at a time.
//seatLock -- it is used in synchronous access to seatAvail.
//seatAvail -- i\it stores the number of seats available for each line.
//seatReserve -- on this condition the helper waits, after reserving a seat,
//so that the customer to sit.
Lock* perLineLock[LINES];
Lock* seatLock = new Lock("SEAT LOCK");
int seatAvail[LINES];
Condition* seatReserve[LINES];
//init, initCond -- this lock and condition ensures the proper initialization
//of all the customers, helpers operator, and safety inspector.
Lock* init = new Lock("INIT LOCK");
Condition* initCond = new Condition("INIT CONDITION");
//safetyLock -- it is used for locking the access to safetyRand.
//safetyRand -- a random value that safety inspector choose to inspect the ride.
//safetyCondition -- using this the safety inspector waits to satrt fixing the
//problem.
//fixedCondition -- using this condition the safety inspector signal the
//operator that the problem has been fixed.
Lock* safetyLock = new Lock("SAFETY LOCK");
Condition* safetyCondition = new Condition("SAFETY CONDITION");
Condition* fixedCondition = new Condition("Fixed CONDITION");
int safetyRand = -2;
//The operator method which satrts a ride. The steps it do is given below.
//step 1) set the current ride number and car number and wake up all the
//helpers to load the cars on eby one.
//step 2) When all the car are loaded it runs the car.
//step 3) After running the car it unload all the customers and start a new
//ride if the number of rides has not finished or there is customer in line.
//Meanwhile it also handle the safety inspector signal if one arrive.
void Operator(){
	init->Acquire();
	initCond->Signal(init);
	init->Release();
	loadLock->Acquire();

	int inSafety = 0;
	int lastLoadedCar = -1;
	int currentRide = -1;
	int nLoadedCars = -1;

	while(true){
		if(inSafety == 1){
			safetyLock->Acquire();
			safetyCondition->Signal(safetyLock);

			fixedCondition->Wait(safetyLock);
			safetyLock->Release();
			printf("%s: Safety Inspector fixed the problem. Next ride is ready to go\n",currentThread->getName());
		}
		custLock->Acquire();
		if(remainingCust == 0){
			custLock->Release();
			loadLock->Release();
			printf("%s: No customer in line. Simulation complete.\n",currentThread->getName());
			break;
		}
		else{
			custLock->Release();
		}

		if((currentRide + 1) == nRide){
			printf("%s: No of rides reached. Simulation complete.\n",currentThread->getName());
			loadLock->Release();
			break;
		}
		currentRide++;
		printf("%s: Ride no %d starts.\n",currentThread->getName(), currentRide + 1);
		while(true){
			custLock->Acquire();
			if(remainingCust == 0){
				custLock->Release();
				currentCar = -1;
				printf("%s: No customer to load.\n",currentThread->getName());
				break;
			}
			else{
				custLock->Release();
			}
			if((currentCar+1) == nCar){
				currentCar = -1;
				printf("%s: All Cars loaded.\n",currentThread->getName());
				break;
			}
			currentCar++;
			nLoadedCars = currentCar;

			seatLock->Acquire();
			for(int i = 0; i < LINES; ++i){
				seatAvail[i] = SEATS_PER_LINE;
			}
			seatLock->Release();

			printf("%s: Loading car %d, wakeup all helpers and wait till car is full.\n",currentThread->getName(), currentCar + 1);
			if(inSafety == 1){
				inSafety = 0;
				loadHelper[(lastLoadedCar + 1)%nCar]->Broadcast(loadLock);
			}
			else{
				loadHelper[currentCar]->Broadcast(loadLock);
			}
			allAboard->Wait(loadLock);
			helperEnd = 0;
			printf("%s: Car %d is loaded.\n",currentThread->getName(), currentCar + 1);

			if((currentCar + nCar*currentRide) == safetyRand){
				lastLoadedCar = currentCar;
				currentCar = -1;
				inSafety = 1;
				printf("%s: Safety Inspector came in, unloading the cars.\n",currentThread->getName());
				break;
			}
		}
		for(int i = 0; i <= nLoadedCars; ++i){
			printf("%s: Car %d has %d customers. Which are: \n",currentThread->getName(), i + 1, seatedCount[i]);
			allUnboard[i]->printWaiting();
		}
		if(inSafety == 0){
			printf("%s: Helpers loaded the cars, now running.\n",currentThread->getName());
			for (int i=0; i<10*nCar; i++ ) {
				currentThread->Yield();
			}
		}
		while(true){
			if(currentCar == nLoadedCars){
				currentCar = nLoadedCars = -1;
				printf("%s: All Cars UnLoaded.\n",currentThread->getName());
				break;
			}
			currentCar++;
			custLock->Acquire();
			allUnboard[currentCar]->Broadcast(custLock);
			lastSafelyUnloaded->Wait(custLock);
			if(inSafety == 0){
				printf("%s: Ride complete for the car %d.\n",currentThread->getName(), currentCar + 1);
			}
			custLock->Release();
		}
		if(inSafety == 0){
			printf("%s: Ride no %d complete.\n",currentThread->getName(), currentRide + 1);
		}
	}
}
//The Customer method which acts like a customer. The steps it do is given below.
//step 1) Choose the shortest line and wait into that line for the helper signal
//to board into a car.
//step 2) after assiging a seat be the helper sit in that sit and wait for the
//operator signal to unboard.
//step 3) after getting the unboard signal, if it is the last customer in the
//car to unboard it send the signal all unboard to the operator.

void Customer(){
	init->Acquire();
	initCond->Signal(init);
	init->Release();

	custLock->Acquire();
	int minLine = 0;
	int minCount = custInline[0];
	for(int i=1; i < LINES; ++i){
		if(custInline[i] < minCount){
			minLine = i;
			minCount = custInline[i];
		}
	}
	custInline[minLine]++;
	printf("%s: Waiting in line %d\n",currentThread->getName(), (minLine + 1));
	inLine[minLine]->Wait(custLock);
	printf("%s: Helper assign a seat in line %d, car %d\n",currentThread->getName(), minLine+1, currentCar + 1);
	seatedCount[currentCar]++;
	remainingCust--;
	seatReserve[minLine]->Signal(custLock);
	printf("%s: Ask helper to fasten seat belt in line %d, car %d.\n",currentThread->getName(), (minLine + 1), currentCar + 1);
	allUnboard[currentCar]->Wait(custLock);
	seatedCount[currentCar]--;
	if(seatedCount[currentCar] == 0){
		printf("%s: All Customers have unloaded the car %d\n",currentThread->getName(), currentCar + 1);
		lastSafelyUnloaded->Signal(custLock);
	}
	custLock->Release();
}
//The Helper method which acts like a helper. The steps it do is given below.
//step 1) wait for the operator signal to load a particular car.
//step 2) after getting the load signal, try to reserve a seat in a line
//and signal the customer in that line to sit and wait.
//step 3) After getting the seat belt fasten signal from the customer fasten
//the seat belt and if the car is full or no customer are there in lines, it
//signal that operator that car is ready to go.
//step 4) it go back and wait for the next car to be loaded.

void Helper(){
	init->Acquire();
	initCond->Signal(init);
	init->Release();
	loadLock->Acquire();
	while(true){
		printf("%s: Waiting for car %d.\n",currentThread->getName(), (currentCar + 1)%nCar + 1);
		loadHelper[(currentCar + 1)%nCar]->Wait(loadLock);
		loadLock->Release();
		printf("%s: Got a new car %d to load.\n",currentThread->getName(), currentCar + 1);
		while(true){
			seatLock->Acquire();
			int line = -1;
			for(int i = 0; i < LINES; ++i){
				if(seatAvail[i] > 0){
					line = i;
					seatAvail[line]--;
					break;
				}
			}
			seatLock->Release();
			if(line > -1){
				printf("%s: Reserved a seat in line %d, car %d.\n",currentThread->getName(), (line + 1), currentCar + 1);
				perLineLock[line]->Acquire();
				//get one customer from line
				custLock->Acquire();
				if(custInline[line] > 0){
					printf("%s: Ask customer to sit in line %d, car %d.\n",currentThread->getName(), (line + 1), currentCar + 1);
					custInline[line]--;
					inLine[line]->Signal(custLock);
					seatReserve[line]->Wait(custLock);
					custLock->Release();
					printf("%s: Seat belt fasten line %d, car %d.\n",currentThread->getName(), (line + 1), currentCar + 1);
				}
				else{
					custLock->Release();
					printf("%s: No customer in line %d, car %d.\n",currentThread->getName(), (line + 1), currentCar + 1);
					seatLock->Acquire();
					seatAvail[line] = 0;
					seatLock->Release();
				}
				perLineLock[line]->Release();


				custLock->Acquire();
				if((seatedCount[currentCar] == LINES*SEATS_PER_LINE) || (remainingCust == 0)){
					custLock->Release();
					printf("%s: Car %d is ready to go.\n",currentThread->getName(), currentCar + 1);
					loadLock->Acquire();
					helperEnd++;
					if(helperEnd == nHelper){
						allAboard->Signal(loadLock);
					}
					break;
				}
				else{
					custLock->Release();
				}
			}
			else{
				printf("%s: Car %d is ready to go.\n",currentThread->getName(), currentCar + 1);
				loadLock->Acquire();
				helperEnd++;
				if(helperEnd == nHelper){
					allAboard->Signal(loadLock);
				}
				break;
			}
		}
	}
}
//The SafetyInspector method which acts like a Safety Inspector.
//The steps it do is given below.
//step 1) randomly set the ride number and car number in that line
//and waits for the operator to unload that ride and respond.
//step 2) after getting the all unload signla from the operator
//it fixes the problem.
//step 3) after fixing the problem it gives signla to operator
//that next ride is ready to go.
void SafetyInspector(){
	init->Acquire();
	safetyLock->Acquire();
	safetyRand = Random() % (nCar*nRide);
	initCond->Signal(init);
	init->Release();
	printf("%s: Safety Inspector is initialized.\n It will wake up in %d ride, after %d car has been loaded.\n",
				currentThread->getName(), (safetyRand/nRide + 1), (safetyRand%nRide + 1));
	safetyCondition->Wait(safetyLock);
	printf("%s: Safety Inspector is fixing the problem.\n",currentThread->getName());
	int fixingtime = Random() % 101 + 50;
	for (int i=0; i<fixingtime; i++ ) {
   		currentThread->Yield();
	}
	printf("%s: Safety Inspector has fixed the problem.\n",currentThread->getName());
	fixedCondition->Signal(safetyLock);
	safetyLock->Release();
}
//setup -- method that do the initial setup like initializing
// variables.
void setup(){
	char *name;
	for(int i = 0; i < LINES; ++i){
		name = new char [20];
		sprintf(name,"LINE CONDITION%d",(i+1));
		inLine[i] = new Condition(name);

		name = new char [20];
		sprintf(name,"SEAT CONDITION%d",(i+1));
		seatReserve[i] = new Condition(name);

		name = new char [20];
		sprintf(name,"PER LINE LOCK%d",(i+1));
		perLineLock[i] = new Lock(name);

		custInline[i] = 0;
	}
	for(int i = 0; i <= nCar; ++i){
		name = new char [20];
		sprintf(name,"LOAD HELPER COND%d",(i+1));
		loadHelper[i] = new Condition(name);

		name = new char [20];
		sprintf(name,"ALL UNBOARD COND%d",(i+1));
		allUnboard[i] = new Condition(name);

		seatedCount[i] = 0;
	}
}
//The test case for the full simulation.
// numbe rof customer = 80
// number of car = 5
// number of helpers = 4
// number of rides = 2
// Safety Inspector enable
void Problem2(){
	setup();
	nCustomer = 80;
	nCar = 5;
	nHelper = 4;
	nRide = 2;

	Thread *t;
	char *name;
	init->Acquire();
    for (int  i = 0 ; i < nCustomer; i++ ) {
		name = new char [20];
		sprintf(name,"CUSTOMER%d",(i+1));
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)Customer,0);
		initCond->Wait(init);
    }

    for (int  i = 0 ; i < nHelper; i++ ) {
		name = new char [20];
		sprintf(name,"HELPER%d",(i+1));
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)Helper,0);
		initCond->Wait(init);
    }

	t = new Thread("SAFETY INSPECTOR");
	t->Fork((VoidFunctionPtr)SafetyInspector,0);
	initCond->Wait(init);

	t = new Thread("OPERATOR");
	t->Fork((VoidFunctionPtr)Operator,0);
	initCond->Wait(init);

	init->Release();
}
//The test case which tests the following things.
//--Customers always take the shortest line, but no 2 Customers ever choose
//the same shortest line at the same time
//--Customers do not get into a Car until told to do so by a Helper
//--Helpers don't tell Customers to load until the Operator tells
//them it is safe to do so

// number of customer = 8
// number of car = 5
// number of helpers = 4
// number of rides = 2
// Safety Inspector not enable

void P2a(){
	setup();
	nCustomer = 8;
	nCar = 5;
	nHelper = 4;
	nRide = 2;

	Thread *t;
	char *name;
	init->Acquire();
	for (int  i = 0 ; i < nCustomer; i++ ) {
		name = new char [20];
		sprintf(name,"CUSTOMER%d",(i+1));
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)Customer,0);
		initCond->Wait(init);
    }
	printf("Customers in lines: \n");

	for(int i = 0; i < LINES; ++i){
		inLine[i]->printWaiting();
	}
    init->Release();
}
//The test case which tests the following things.
//--No Customer can be told by two different Helpers to get into a Car
//--Customers are never abandoned and left on cars - even if there is no
//one waiting to get on another Car
//--A Car is never sent out on the track until all Customer are safely loaded
//--Helpers don't tell Customers to load until the Operator tells them it is safe to do so
//--No Car is unloaded until the just-loaded Car has left the loading area
//--Customers never exit a Car until told to do so by the Operator

// number of customer = 20
// number of car = 3
// number of helpers = 2
// number of rides = 1
// Safety Inspector not enable

void P2b(){
	setup();
	nCustomer = 20;
	nCar = 3;
	nHelper = 2;
	nRide = 1;

	Thread *t;
	char *name;
	init->Acquire();
    for (int  i = 0 ; i < nCustomer; i++ ) {
		name = new char [20];
		sprintf(name,"CUSTOMER%d",(i+1));
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)Customer,0);
		initCond->Wait(init);
    }

    for (int  i = 0 ; i < nHelper; i++ ) {
		name = new char [20];
		sprintf(name,"HELPER%d",(i+1));
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)Helper,0);
		initCond->Wait(init);
    }

	t = new Thread("OPERATOR");
	t->Fork((VoidFunctionPtr)Operator,0);
	initCond->Wait(init);

	init->Release();
}
//The test case which tests the following things.
//--No Cars are loaded when a Safety Inspector decides a ride is unsafe
//AND all Customers on the ride are allowed to get out of the Car they are in.

// number of customer = 20
// number of car = 3
// number of helpers = 2
// number of rides = 1
// Safety Inspector enable

void P2c(){
	setup();
	nCustomer = 20;
	nCar = 3;
	nHelper = 2;
	nRide = 1;

	Thread *t;
	char *name;
	init->Acquire();
    for (int  i = 0 ; i < nCustomer; i++ ) {
		name = new char [20];
		sprintf(name,"CUSTOMER%d",(i+1));
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)Customer,0);
		initCond->Wait(init);
    }

    for (int  i = 0 ; i < nHelper; i++ ) {
		name = new char [20];
		sprintf(name,"HELPER%d",(i+1));
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)Helper,0);
		initCond->Wait(init);
    }

	t = new Thread("SAFETY INSPECTOR");
	t->Fork((VoidFunctionPtr)SafetyInspector,0);
	initCond->Wait(init);

	t = new Thread("OPERATOR");
	t->Fork((VoidFunctionPtr)Operator,0);
	initCond->Wait(init);

	init->Release();
}

#endif
