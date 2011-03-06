// progtest.cc
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"
#include "string.h"

#define QUANTUM 100

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void execStartthread(){
    currentThread->space->RestoreState();		// load page table register
    currentThread->space->InitRegisters(false);		// set the initial register values
	machine->Run();		// jump to the user progam
}
void
StartProcess(char *filename)
{
    AddrSpace *space = new AddrSpace();
    space->executable = fileSystem->Open(filename);

    if (space->executable == NULL) {
		delete space;
		printf("Unable to open file %s\n", filename);
		return;
    }
	int id;
	if ((id = processesTable->Put(space)) == -1 ){
		delete space;
		printf("Unable to create child process %s\n", filename);
		return;
	}
	space->pinfo.spaceId = id;

	if( !space->load()){
		delete space;
		printf("Unable to load file %s\n", filename);
		return;
	}

	Thread* execThread = new Thread(filename);
    execThread->space = space;
    sysLock->Acquire();
    space->pinfo.id = ++genId;
    execThread->mailBoxNo = totalThreads;
	totalThreads++;
	space->pinfo.nThreads++;
	DEBUG('v', "EXEC: %s -> %s\n", currentThread->getName(), filename);
	DEBUG('v', "space id= %d, totalThreads= %d, nThreads= %d\n", space->pinfo.spaceId, totalThreads, space->pinfo.nThreads);
    sysLock->Release();

	machine->WriteRegister(2, id);
	execThread->Fork((VoidFunctionPtr)execStartthread, 0);
    //ASSERT(FALSE);	// machine->Run never returns; the address space exits by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);

    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}

