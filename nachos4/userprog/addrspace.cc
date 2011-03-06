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
#ifdef CHANGED
AddrSpace::AddrSpace() : fileTable(MaxOpenFiles),lockTable(MaxLocks),conditionTable(MaxConditions),stacks(MaxThreads) {
    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);
    pageTable = NULL;
	pinfo.nThreads = 0;
	mark4Deletion = false;
}

void AddrSpace::pageTableFillup(int filestart, int len){
    unsigned int vpn = 0;
    unsigned int fileOffset = 40;//filestart;
    int toCopy = len;

    while(toCopy>0){
		if(PageSize > toCopy){
			//mixed
			//printf("mixed->vpn->%d\n", vpn);
			pageTable[vpn].pageType = 2;
			pageTable[vpn].exelocation = fileOffset;
			pageTable[vpn].dataSize = toCopy;
			pageTable[vpn].readOnly = FALSE;
			break;
		} else{
			//exe
			//printf("exe->vpn->%d\n", vpn);
			pageTable[vpn].pageType = 1;
			pageTable[vpn].exelocation = fileOffset;
			pageTable[vpn].readOnly = TRUE;
		}
    	fileOffset += PageSize;
    	toCopy -= PageSize;
    	vpn++;
	}
}

bool AddrSpace::load(){
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

	size = noffH.code.size + noffH.initData.size + noffH.uninitData.size;
	numPages = divRoundUp(size, PageSize) + divRoundUp(UserStackSize, PageSize) * MaxThreads;

	DEBUG('a',"Initializing address space, num pages %d, size %d\n",numPages, numPages * PageSize);
	// first, set up the translation
	pageTable = new TranslationEntryPT[numPages];
	for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;
		pageTable[i].physicalPage = -1;
		pageTable[i].valid = FALSE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;
		pageTable[i].pageType = 0; //swap data by default
		pageTable[i].swaplocation = -1;
		pageTable[i].exelocation = -1;
		pageTable[i].dataSize = PageSize;
	}
	pageTableFillup(noffH.code.inFileAddr, noffH.code.size+noffH.initData.size);
	return true;
}

bool AddrSpace::readExe(int ppn, int vpn){
	ASSERT(pageTable[vpn].pageType == 1);
	int start = ppn * PageSize;
   	if( PageSize != executable->ReadAt(machine->mainMemory + start,
   		PageSize, pageTable[vpn].exelocation)){
		printf("ERROR: The executable read problem in exe page.\n");
		return false;
	}
	return true;
}
bool AddrSpace::readSwap(int ppn, int vpn){
	ASSERT(pageTable[vpn].pageType == 0);
	int start = ppn * PageSize;
	int fileStart = pageTable[vpn].swaplocation*PageSize;
	if(pageTable[vpn].dirty == TRUE){
		ASSERT(pageTable[vpn].swaplocation >= 0);
   		if( PageSize != swapFile->ReadAt(machine->mainMemory + start, PageSize, fileStart)){
			printf("ERROR: The SWAP read problem in swap page.\n");
			return false;
		}
	}
	return true;
}

bool AddrSpace::writeSwap(int ppn, int vpn){
	ASSERT(pageTable[vpn].pageType == 0);
	int start = ppn * PageSize;
	int fileStart = pageTable[vpn].swaplocation*PageSize;
	pageTable[vpn].dirty = TRUE;

   	if( PageSize != swapFile->WriteAt(machine->mainMemory + start, PageSize, fileStart)){
		printf("ERROR: The SWAP write problem in swap.\n");
		return false;
	}
	return true;
}

bool AddrSpace::writeBack(int ppn, int vpn){
	if(pageTable[vpn].pageType != 1){
		pageTable[vpn].swaplocation = swapPages->Find();
		ASSERT(pageTable[vpn].swaplocation != -1);
	}
	//printf("write->ppn %d->vpn %d -> swaplocation %d->pageType %d\n", ppn, vpn, pageTable[vpn].swaplocation,pageTable[vpn].pageType);
	bool ret = FALSE;
	switch(pageTable[vpn].pageType){
		case 0:	ret = writeSwap(ppn, vpn);break;
		case 1: ret = TRUE;break;
		case 2:	ret = writeMix(ppn, vpn);break;
	}
	pageTable[vpn].physicalPage = -1;
	return ret;
}
bool AddrSpace::readFrom(int ppn, int vpn){
	ASSERT(vpn >= 0 && vpn < (int)numPages);
	//printf("read->ppn %d->vpn %d -> swaplocation %d->pageType %d\n", ppn, vpn, pageTable[vpn].swaplocation,pageTable[vpn].pageType);
	bool ret = FALSE;
	pageTable[vpn].physicalPage = ppn;
	bzero(machine->mainMemory + ppn * PageSize, PageSize);
	switch(pageTable[vpn].pageType){
		case 0: ret = readSwap(ppn, vpn);break;
		case 1: ret = readExe(ppn, vpn);break;
		case 2: ret = readMix(ppn, vpn);break;
	}
	if((pageTable[vpn].pageType != 1) && (pageTable[vpn].swaplocation != -1)){
		swapPages->Clear(pageTable[vpn].swaplocation);
		pageTable[vpn].swaplocation = -1;
	}
	ipt[ppn].readOnly = pageTable[vpn].readOnly;
	return ret;
}

bool AddrSpace::readMix(int ppn, int vpn){
	ASSERT(pageTable[vpn].pageType == 2);

	int start = ppn * PageSize;
   	if( pageTable[vpn].dataSize != executable->ReadAt(machine->mainMemory + start,
   	    pageTable[vpn].dataSize, pageTable[vpn].exelocation)){
		printf("ERROR: The executable read problem.\n");
		return false;
	}
	start += pageTable[vpn].dataSize;
	int fileStart = pageTable[vpn].swaplocation*PageSize + pageTable[vpn].dataSize;
	if(pageTable[vpn].dirty == TRUE){
		if( (PageSize-pageTable[vpn].dataSize) != swapFile->ReadAt(machine->mainMemory + start,
			(PageSize-pageTable[vpn].dataSize), fileStart)){
			printf("ERROR: The SWAP read problem in mix page.\n");
			return false;
		}
	}
	return true;
}
bool AddrSpace::writeMix(int ppn, int vpn){
	ASSERT(pageTable[vpn].pageType == 2);
	int start = ppn * PageSize + pageTable[vpn].dataSize;
	int fileStart = pageTable[vpn].swaplocation*PageSize + pageTable[vpn].dataSize;
	pageTable[vpn].dirty = TRUE;
	if( (PageSize-pageTable[vpn].dataSize) != swapFile->WriteAt(machine->mainMemory + start,
		(PageSize-pageTable[vpn].dataSize), fileStart)){
		printf("ERROR: The SWAP write problem in mix page.\n");
		return false;
	}
	return true;
}

void AddrSpace::clearStack(){
	int vaddr = machine->ReadRegister(StackReg);
	int si = (numPages * PageSize - vaddr - 16)/UserStackSize;
	int stackpage = divRoundUp(UserStackSize,PageSize);

	for (int i = 0; i < stackpage; i++) {
		//release swap space
		if(pageTable[numPages - si*stackpage - i - 1].swaplocation != -1){
			swapPages->Clear(pageTable[(numPages - si*stackpage) - i-1].swaplocation);
		}
	}
	stacks.Clear(si);
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
		sysLock->Acquire();
		for (unsigned int i = 0; i < numPages; i++) {
			//release swap space
			if(pageTable[i].swaplocation != -1){
				swapPages->Clear(pageTable[i].swaplocation);
			}
			//release IPT
			if(pageTable[i].physicalPage != -1){
				IPTPages->Clear(pageTable[i].physicalPage);
				ipt[pageTable[i].physicalPage].use = FALSE;
			}
		}
		delete[] pageTable;
		delete executable;
		sysLock->Release();
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
	sysLock->Acquire();
	int si = stacks.Find();
	ASSERT(-1 != si);
    machine->WriteRegister(StackReg, (numPages * PageSize - UserStackSize*si)- 16);
    sysLock->Release();
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState(){
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	for(int i = 0; i < TLBSize; ++i){
		machine->tlb[i].valid = FALSE;
	}
	(void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState()
{
    //machine->pageTable = pageTable;
    //machine->pageTableSize = numPages;
}
