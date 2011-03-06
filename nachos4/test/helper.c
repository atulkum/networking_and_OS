#include "syscall.h"

void helper(){
	int isDone = 0;
	HRegister();
	Write("Waiting for car.\n", 17, ConsoleOutput);
	for(;;){
		HWaitForCar();
		Write("Got a new car to load.\n",23,ConsoleOutput);
		while(isDone == 0){
			Write("Got a cust to load.   \n",23,ConsoleOutput);
			isDone = HSeatCustomer();
		}
	}
	Exit(0);
}
int main(){
	Write("Helpers.\n", 9, ConsoleOutput);
	Fork(helper);
	Fork(helper);

	Exit(0);
}
