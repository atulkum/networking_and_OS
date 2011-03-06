#include "syscall.h"

void customer(){
	int line;
	Write("Customer waiting in line.\n",26,ConsoleOutput);
	line = CGetInLine();
	CWaitForHelper();
	Write("Seating in the car.      \n",26,ConsoleOutput);
	CWaitForUnload(line);
	Write("Unloading the car.       \n",26,ConsoleOutput);
	Exit(0);
}

int main(){
	Write("5 Customers.\n", 13, ConsoleOutput);
	Fork(customer);
	Fork(customer);
	Fork(customer);
	Fork(customer);
	Fork(customer);
	Exit(0);
}
