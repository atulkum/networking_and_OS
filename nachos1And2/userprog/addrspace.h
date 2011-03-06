// addrspace.h
//	Data structures to keep track of executing user programs
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "table.h"

#define UserStackSize		1024 	// increase this as necessary!

#define MaxOpenFiles 256
#define MaxChildSpaces 256

#ifdef CHANGED
#define MaxLocks 32
#define MaxConditions 32
#define MaxThreads 16
typedef struct processInfo{
  unsigned int id;
  unsigned int nThreads;
  unsigned int spaceId;
} ProcessInfo;
#endif
class AddrSpace {
  public:
    AddrSpace();	// Create an address space,
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters(bool isFork);		// Initialize user-level CPU registers,
								// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch
    Table fileTable;			// Table of openfiles
#ifdef CHANGED
	bool load(OpenFile *executable);// load the address space with the program
									// stored in the file "executable"
	Table lockTable;
	Table conditionTable;
	ProcessInfo pinfo;
	BitMap stacks;
	bool mark4Deletion;
	void clearStack();
#endif

 private:
 	bool copyPageByPage(OpenFile *executable, int fileStart, int len, int virtualAdd );
    TranslationEntry *pageTable;// Assume linear page table translation for now!
    unsigned int numPages;		// Number of pages in the virtual address space
    unsigned int exePages;
    unsigned int stackPages;
};

#endif // ADDRSPACE_H
