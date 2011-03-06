// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "table.h"

extern "C" { int bzero(char *, int); };

Table::Table(int s) : map(s), table(0), lock(0), size(s) {
    table = new void *[size];
    lock = new Lock("TableLock");
}

Table::~Table() {
    if (table) {
	delete table;
	table = 0;
    }
    if (lock) {
	delete lock;
	lock = 0;
    }
}

void *Table::Get(int i) {
    // Return the element associated with the given if, or 0 if
    // there is none.

    return (i >=0 && i < size && map.Test(i)) ? table[i] : 0;
}

int Table::Put(void *f) {
    // Put the element in the table and return the slot it used.  Use a
    // lock so 2 files don't get the same space.
    int i;	// to find the next slot

    lock->Acquire();
    i = map.Find();
    lock->Release();
    if ( i != -1)
	table[i] = f;
    return i;
}

void *Table::Remove(int i) {
    // Remove the element associated with identifier i from the table,
    // and return it.

    void *f =0;

    if ( i >= 0 && i < size ) {
	lock->Acquire();
	if ( map.Test(i) ) {
	    map.Clear(i);
	    f = table[i];
	    table[i] = 0;
	}
	lock->Release();
    }
    return f;
}

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	"executable" is the file containing the object code to load into memory
//
//      It's possible to fail to fully construct the address space for
//      several reasons, including being unable to allocate memory,
//      and being unable to read key parts of the executable.
//      Incompletely consretucted address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------
AddrSpace::AddrSpace() : fileTable(MaxOpenFiles),lockTable(MaxLocks),conditionTable(MaxConditions),stacks(MaxThreads) {
    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);
    pageTable = NULL;
	pinfo.nThreads = 0;
	mark4Deletion = false;
}
#ifdef CHANGED

void AddrSpace::clearStack(){
	int vaddr = machine->ReadRegister(StackReg);
	int si = (numPages * PageSize - vaddr - 16)/UserStackSize;
	int nStackPage = divRoundUp(UserStackSize,PageSize);
	int offset = numPages - 1 - nStackPage*si;

	for (int i = 0; i < nStackPage; ++i) {
		physicalMemPages->Clear(pageTable[offset - i].physicalPage);
		pageTable[offset - i].valid = FALSE;
	}
	stacks.Clear(si);
}
bool AddrSpace::copyPageByPage(OpenFile *executable, int fileStart, int len, int virtAddr ){
    unsigned int vpn, offset, emptyInPage, toCopy, fileOffset, start;
	vpn = (unsigned) virtAddr / PageSize;
    offset = (unsigned) virtAddr % PageSize;
	emptyInPage = PageSize - offset;
	toCopy = len;
    fileOffset = fileStart;
    while(toCopy>0){
		if(emptyInPage > toCopy){
			start = pageTable[vpn].physicalPage * PageSize + offset;
	    	if(toCopy != executable->ReadAt(machine->mainMemory + start, toCopy, fileOffset)){
				printf("ERROR: The executable read problem.\n");
				return false;
			}
			break;
		}
		else{
			start = pageTable[vpn].physicalPage * PageSize + offset;
	    	if( emptyInPage != executable->ReadAt(machine->mainMemory + start, emptyInPage, fileOffset)){
				printf("ERROR: The executable read problem.\n");
				return false;
			}
	    	fileOffset += emptyInPage;
	    	toCopy -= emptyInPage;
	    	vpn++;
	    	emptyInPage = PageSize;
	    	offset=0;
		}
	}
    return true;
}
bool AddrSpace::load(OpenFile *executable){
	NoffHeader noffH;
	unsigned int i, size;
	executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
	if ((noffH.noffMagic != NOFFMAGIC) &&
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
		SwapHeader(&noffH);
	if(noffH.noffMagic != NOFFMAGIC){
		printf("ERROR: The executable file is not in NOCHOS FILE FORMAT.\n");
		return false;
	}

	size = noffH.code.size + noffH.initData.size + noffH.uninitData.size ;
	exePages = divRoundUp(size, PageSize);
	stackPages = divRoundUp(UserStackSize,PageSize) * MaxThreads;
	numPages = exePages + stackPages;

	phyMemLock->Acquire();
	int pageavail = physicalMemPages->NumClear();
	if(pageavail < (int)numPages){
		printf("ERROR: Number of pages requied(%d) is more than the free physical memory pages available(%d).\n", numPages, pageavail );
		phyMemLock->Release();
		ASSERT(FALSE);
	}

	DEBUG('a', "Initializing address space, num pages %d, size %d\n",numPages, numPages * PageSize);
	// first, set up the translation
	pageTable = new TranslationEntry[numPages];
	//for (i = 0; i < numPages; i++) {
	for (i = 0; i < exePages; i++) {
		pageTable[i].virtualPage = i;
		pageTable[i].physicalPage = physicalMemPages->Find();
		// zero out the entire page
		bzero((machine->mainMemory + (pageTable[i].physicalPage * PageSize)), PageSize);
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;
	}
	phyMemLock->Release();
	// copy in the code and data segments into memory
	if (noffH.code.size > 0) {
		DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", noffH.code.virtualAddr, noffH.code.size);
		if(!copyPageByPage(executable, noffH.code.inFileAddr, noffH.code.size, noffH.code.virtualAddr)){
			printf("ERROR: The executable read problem.\n");
			return false;
		}
	}
	if (noffH.initData.size > 0) {
		DEBUG('a', "Initializing init data segment, at 0x%x, size %d\n", noffH.initData.virtualAddr, noffH.initData.size);
		if(!copyPageByPage(executable, noffH.initData.inFileAddr, noffH.initData.size, noffH.initData.virtualAddr)){
			printf("ERROR: The executable read problem.\n");
			return false;
		}
	}

	return true;
}
#endif
//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
// 	Dealloate an address space.  release pages, page tables, files
// 	and file tables
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
	if(pageTable != NULL) {
		phyMemLock->Acquire();
		for (unsigned int i = 0; i < numPages; i++) {
			if(pageTable[i].valid == TRUE){
				physicalMemPages->Clear(pageTable[i].physicalPage);
			}
		}
		phyMemLock->Release();
		delete pageTable;
	}
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters(bool isFork)
{
	if(!isFork){
		int i;

		for (i = 0; i < NumTotalRegs; i++)
		machine->WriteRegister(i, 0);

		// Initial program counter -- must be location of "Start"
		machine->WriteRegister(PCReg, 0);

		// Need to also tell MIPS where next instruction is, because
		// of branch delay possibility
		machine->WriteRegister(NextPCReg, 4);
	}
	phyMemLock->Acquire();
	int si = stacks.Find();
	ASSERT(-1 != si);

	int nStackPage = divRoundUp(UserStackSize,PageSize);
	int offset = numPages - 1 - nStackPage*si;

	for (int i = 0; i < nStackPage; ++i) {
		pageTable[offset - i].virtualPage = offset - i;
		pageTable[offset - i].physicalPage = physicalMemPages->Find();
		ASSERT(-1 != pageTable[offset - i].physicalPage);
		// zero out the entire page
		bzero((machine->mainMemory + (pageTable[offset - i].physicalPage * PageSize)), PageSize);
		pageTable[offset - i].valid = TRUE;
		pageTable[offset - i].use = FALSE;
		pageTable[offset - i].dirty = FALSE;
		pageTable[offset - i].readOnly = FALSE;
	}

	phyMemLock->Release();
    machine->WriteRegister(StackReg, (numPages * PageSize - UserStackSize*si)- 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState(){}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState()
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
