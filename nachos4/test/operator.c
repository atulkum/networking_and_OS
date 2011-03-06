#include "syscall.h"

int main(){
	int i;
	Write("Operator starts ride.\n", 22, ConsoleOutput);
	OStartRide();
	Write("Now Running.  \n",15,ConsoleOutput);
	for (i = 0; i<10; i++ ) {
		Yield();
	}
	Write("start Unloading.\n",17,ConsoleOutput);
	OFinishRide();
	Write("Ride Finished.\n",15,ConsoleOutput);

	Exit(0);
}