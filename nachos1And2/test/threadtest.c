#include "syscall.h"

int helperEnd = 0;
int seatedCount = 0;
int custInline[4] = {0,0,0,0};
int seatAvail[4] = {2,2,2,2};

int perLineLock[4];
int inLine[4];
int seatReserve[4];

int loadHelper,loadLock, allAboard, lastSafelyUnloaded,allUnboard, custLock, seatLock, init, initCond;

void Operator(){
	int i=0;
	Acquire(loadLock);
	Write("Operator Signal Helpers.\n",25,ConsoleOutput);
	Broadcast(loadHelper, loadLock);
	Wait(allAboard, loadLock);

	Release(loadLock);
	Write("Car is loaded.\n",15,ConsoleOutput);
	PrintC(allUnboard);

	Write("Now Running.  \n",15,ConsoleOutput);
	for (; i<10; i++ ) {
		Yield();
	}
	Write("start Unloading.\n",17,ConsoleOutput);
	Acquire(custLock);
	Broadcast(allUnboard, custLock);
	Wait(lastSafelyUnloaded, custLock);
	Release(custLock);
	Write("Ride Finished.\n",15,ConsoleOutput);

	Acquire(init);
	Signal(initCond, init);
	Release(init);

	Exit(0);
}
void Customer(){
	int minLine;
	int minCount;
	int i;

	Acquire(init);
	Signal(initCond, init);
	Release(init);

	Acquire(custLock);
	minCount = custInline[0];
	minLine = 0;
	for(i=1; i < 4; ++i){
		if(custInline[i] < minCount){
			minLine = i;
			minCount = custInline[i];
		}
	}
	custInline[minLine]++;
	Write("Waiting in line.        \n",25, ConsoleOutput);
	Wait(inLine[minLine], custLock);

	Write("Helper assign a seat.   \n",25, ConsoleOutput);
	seatedCount++;

	Signal(seatReserve[minLine], custLock);
	Write("Ask helper for seatbelt.\n",25,ConsoleOutput);
	Wait(allUnboard,custLock);
	seatedCount--;
	if(seatedCount == 0){
		Write("Car Unloaded.\n",14,ConsoleOutput);
		Signal(lastSafelyUnloaded, custLock);
	}
	Release(custLock);

	Exit(0);
}
void Helper(){
	int line;
	int i;
	Acquire(init);
	Signal(initCond, init);
	Release(init);
	Write("Waiting for car.\n", 17, ConsoleOutput);
	Acquire(loadLock);
	Wait(loadHelper, loadLock);
	Release(loadLock);
	Write("Got a new car to load.\n",23,ConsoleOutput);
	for(;;){
		Acquire(seatLock);
		line = -1;
		for(i = 0; i < 4; ++i){
			if(seatAvail[i] > 0){
				line = i;
				seatAvail[line]--;
				break;
			}
		}
		Release(seatLock);
		if(line > -1){
			Acquire(perLineLock[line]);
			Acquire(custLock);
			Write("Ask customer to sit.\n",21, ConsoleOutput);
			custInline[line]--;
			Signal(inLine[line], custLock);
			Wait(seatReserve[line], custLock);
			Release(custLock);
			Write("Seat belt fasten.\n",18,ConsoleOutput);
			Release(perLineLock[line]);
			Acquire(custLock);
			if(seatedCount == 8){
				Release(custLock);
				Write("Car is ready to go.\n", 20, ConsoleOutput);
				Acquire(loadLock);
				helperEnd++;
				if(helperEnd == 2){
					Signal(allAboard, loadLock);
				}
				Release(loadLock);
				break;
			}
			else{
				Release(custLock);
			}
		}
		else{
			Write("Car is ready to go.\n", 20, ConsoleOutput);
			Acquire(loadLock);
			helperEnd++;
			if(helperEnd == 2){
				Signal(allAboard, loadLock);
			}
			Release(loadLock);
			break;
		}
	}
	Exit(0);
}
int main(){
	Write( "Roller Coaster Ride.\n", 21, ConsoleOutput );

	init = CreateLock("INIT LOCK", 9);
	initCond = CreateCondition("INIT CONDITION", 14);
	inLine[0] = CreateCondition("LINE CONDITION1",15);
	inLine[1] = CreateCondition("LINE CONDITION2",15);
	inLine[2] = CreateCondition("LINE CONDITION3",15);
	inLine[3] = CreateCondition("LINE CONDITION4",15);
	seatReserve[0] = CreateCondition("SEAT CONDITION1", 15);
	seatReserve[1] = CreateCondition("SEAT CONDITION2", 15);
	seatReserve[2] = CreateCondition("SEAT CONDITION3", 15);
	seatReserve[3] = CreateCondition("SEAT CONDITION4", 15);
	perLineLock[0] = CreateLock("PER LINE LOCK1", 14);
	perLineLock[1] = CreateLock("PER LINE LOCK2", 14);
	perLineLock[2] = CreateLock("PER LINE LOCK3", 14);
	perLineLock[3] = CreateLock("PER LINE LOCK4", 14);
	loadHelper = CreateCondition("LOAD HELPER COND1", 17);
	allUnboard = CreateCondition("ALL UNBOARD COND", 16);

	loadLock = CreateLock("LOAD LOCK", 9);
	allAboard = CreateCondition("ALL ABOARD COND", 15);
	custLock = CreateLock("CUSTOMER LOCK", 13);
	lastSafelyUnloaded = CreateCondition("LAST SAFELY UNLOAD COND", 23);
	seatLock = CreateLock("SEAT LOCK", 9);

	Acquire(init);
	Fork(Customer);
	Wait(initCond, init);
	Fork(Customer);
	Wait(initCond, init);
	Fork(Customer);
	Wait(initCond, init);
	Fork(Customer);
	Wait(initCond, init);
	Fork(Customer);
	Wait(initCond, init);
	Fork(Customer);
	Wait(initCond, init);
	Fork(Customer);
	Wait(initCond, init);
	Fork(Customer);
	Wait(initCond, init);
	Write("CUST INIT\n", 10, ConsoleOutput);
	Fork(Helper);
	Wait(initCond, init);
	Fork(Helper);
	Wait(initCond, init);
	Write("HELP INIT\n", 10, ConsoleOutput);
	Fork(Operator);
	Wait(initCond, init);
	Release(init);

	DestroyLock(init);
	DestroyCondition(initCond);
	DestroyCondition(inLine[0]);
	DestroyCondition(inLine[1]);
	DestroyCondition(inLine[2]);
	DestroyCondition(inLine[3]);
	DestroyCondition(seatReserve[0]);
	DestroyCondition(seatReserve[1]);
	DestroyCondition(seatReserve[2]);
	DestroyCondition(seatReserve[3]);
	DestroyLock(perLineLock[0]);
	DestroyLock(perLineLock[1]);
	DestroyLock(perLineLock[2]);
	DestroyLock(perLineLock[3]);
	DestroyCondition(loadHelper);
	DestroyCondition(allUnboard);
	DestroyLock(loadLock);
	DestroyCondition(allAboard);
	DestroyLock(custLock);
	DestroyCondition(lastSafelyUnloaded);
	DestroyLock(seatLock);


	Exit(0);
}
