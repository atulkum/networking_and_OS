// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <stdio.h>
#include <iostream>
#include "synch.h"
#include <sys/time.h>
#include <unistd.h>

using namespace std;
int readFromPhysicalMem(int vaddr, char *buf){
	bool result;
	int len=0;			// The number of bytes copied in
	int *paddr = new int;

	while ( len < MAXFILENAME) {
		ASSERT(vaddr >= 0);
	  result = machine->ReadMem( vaddr, 1, paddr );
	  if ( !result ) {
	  	//delete paddr;
	  	//return -1;
	  	continue;
	  }
	  buf[len] = *paddr;

	  if('\0' == buf[len]){
		  break;
	  }
	  len++;
	  vaddr++;
	}

	delete paddr;
	if (len == MAXFILENAME){
		return -1;
	}
	return len;
}

int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;			// The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
		ASSERT(vaddr >= 0);
      result = machine->ReadMem( vaddr, 1, paddr );
      buf[n] = *paddr;

      if ( !result ) {
	//translation failed

	continue;
	//return -1;
      }
	  n++;
      vaddr++;
    }

    delete paddr;
    return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
    bool result;
    int n=0;			// The number of bytes copied in

    while ( n >= 0 && n < len) {
      // Note that we check every byte's address
      result = machine->WriteMem( vaddr, 1, (int)(buf[n]) );

      if ( !result ) {
	//translation failed
	continue;
	//return -1;
      }
      n++;
      vaddr++;
    }

    return n;
}

void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];	// Kernel buffer to put the name in

    if (!buf) return;

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Create\n");
	delete buf;
	return;
    }

    buf[len]='\0';

    fileSystem->Create(buf,0);
    delete[] buf;
    return;
}

int Open_Syscall(unsigned int vaddr, int len) {
    // Open the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  If
    // the file is opened successfully, it is put in the address
    // space's file table and an id returned that can find the file
    // later.  If there are any errors, -1 is returned.
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    OpenFile *f;			// The new open file
    int id;				// The openfile id

    if (!buf) {
	printf("%s","Can't allocate kernel buffer in Open\n");
	return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Open\n");
	delete[] buf;
	return -1;
    }

    buf[len]='\0';

    f = fileSystem->Open(buf);
    delete[] buf;

    if ( f ) {
	if ((id = currentThread->space->fileTable.Put(f)) == -1 )
	    delete f;
	return id;
    }
    else
	return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.

    char *buf;		// Kernel buffer for output
    OpenFile *f;	// Open file for output

    if ( id == ConsoleInput) return;

    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
	    printf("%s","Bad pointer passed to to write: data not written\n");
	    delete[] buf;
	    return;
	}
    }

    if ( id == ConsoleOutput) {
      for (int ii=0; ii<len; ii++) {
	printf("%c",buf[ii]);
      }

    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    f->Write(buf, len);
	} else {
	    printf("%s","Bad OpenFileId passed to Write\n");
	    len = -1;
	}
    }

    delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one.    We reuse len as the number of bytes
    // read, which is an unnessecary savings of space.
    char *buf;		// Kernel buffer for input
    OpenFile *f;	// Open file for output

    if ( id == ConsoleOutput) return -1;

    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer in Read\n");
	return -1;
    }

    if ( id == ConsoleInput) {
      //Reading from the keyboard
      scanf("%s", buf);

      if ( copyout(vaddr, len, buf) == -1 ) {
	printf("%s","Bad pointer passed to Read: data not copied\n");
      }
    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    len = f->Read(buf, len);
	    if ( len > 0 ) {
	        //Read something from the file. Put into user's address space
  	        if ( copyout(vaddr, len, buf) == -1 ) {
		    printf("%s","Bad pointer passed to Read: data not copied\n");
		}
	    }
	} else {
	    printf("%s","Bad OpenFileId passed to Read\n");
	    len = -1;
	}
    }

    delete[] buf;
    return len;
}

void Close_Syscall(int fd) {
    // Close the file associated with id fd.  No error reporting.
    OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

    if ( f ) {
      delete f;
    } else {
      printf("%s","Tried to close an unopen file\n");
    }
}
#ifdef CHANGED
void exec_thread(){
    currentThread->space->RestoreState();		// load page table register
    currentThread->space->InitRegisters(false);		// set the initial register values
	machine->Run();		// jump to the user progam
}

int Exec_Syscall(unsigned int vaddr){
   	char *buf = new char[MAXFILENAME];	// Kernel buffer to put the name in
   	if (!buf) return -1;
   	if( readFromPhysicalMem(vaddr, buf) == -1 ) {
		printf("Bad pointer passed to Create\n");
		delete buf;
		return -1;
	}

    AddrSpace *space = new AddrSpace();

	space->executable = fileSystem->Open(buf);
    if (space->executable == NULL) {
		delete space;
		printf("Unable to open file %s\n", buf);
		return - 1;
    }
	int id;
	if ((id = processesTable->Put(space)) == -1 ){
		delete space;
		printf("Unable to create child process %s\n", buf);
		return -1;
	}
	space->pinfo.spaceId = id;
	if( !space->load()){
		delete space;
		printf("Unable to load file %s\n", buf);
		return -1;
	}

	Thread* execThread = new Thread(buf);
    execThread->space = space;
    sysLock->Acquire();
    space->pinfo.id = ++genId;
    execThread->mailBoxNo = totalThreads;
	totalThreads++;
	space->pinfo.nThreads++;
	DEBUG('v', "space id= %d, totalThreads= %d, nThreads= %d\n",
		space->pinfo.spaceId, totalThreads, space->pinfo.nThreads);
    sysLock->Release();
	machine->WriteRegister(2, id);
	execThread->Fork((VoidFunctionPtr)exec_thread, 0);
	return 0;
}
void kernel_thread(unsigned int vaddr) {
	currentThread->space->RestoreState();
    currentThread->space->InitRegisters(true);
    machine->WriteRegister(PCReg,vaddr);
	machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
	machine->Run();
}
void Fork_Syscall(unsigned int vaddr){
	if(currentThread->space->pinfo.nThreads == MaxThreads){
		printf("Max thread reached.\n");
		ASSERT(FALSE);
	}
	if(currentThread->space->mark4Deletion == true){
		printf("Process is marked for deletion.\n");
		return;
	}
	sysLock->Acquire();
	int mailbox = totalThreads;
	totalThreads++;
	currentThread->space->pinfo.nThreads++;
	DEBUG('v', "space id= %d, totalThreads= %d, nThreads= %d\n",
		currentThread->space->pinfo.spaceId, totalThreads,
		currentThread->space->pinfo.nThreads);

	sysLock->Release();
	char *name = new char [MAXFILENAME];
	sprintf(name,"%s%d",currentThread->getName(), currentThread->space->pinfo.nThreads);
	DEBUG('v', "FORK: %s -> %s\n", currentThread->getName(), name);
	Thread* kernalThread = new Thread(name);
	kernalThread->mailBoxNo = mailbox;
    kernalThread->space = currentThread->space;
    kernalThread->Fork((VoidFunctionPtr)kernel_thread, vaddr);
    return;
}

void Exit_Syscall(unsigned int status){
	//printf("%s: EXIT CODE : %d\n", currentThread->getName(), status);
	printf("EXIT CODE : %d\n", status);
	printf("Thread with mailbox number %d exiting.\n", currentThread->mailBoxNo);
	sysLock->Acquire();
	currentThread->space->clearStack();
	totalThreads--;
	currentThread->space->pinfo.nThreads--;
	DEBUG('v', "EXIT: %s\n", currentThread->getName());
	DEBUG('v', "space id= %d, totalThreads= %d, nThreads= %d\n",
		currentThread->space->pinfo.spaceId, totalThreads,
		currentThread->space->pinfo.nThreads);
	if(0 == totalThreads){
		if(currentThread->space){
			currentThread->space->mark4Deletion = true;
			sysCond->Broadcast(sysLock);
			sysLock->Release();
			processesTable->Remove(currentThread->space->pinfo.spaceId);
			delete currentThread->space;
		}
		else{
			sysLock->Release();
		}
		currentThread->Finish();
		interrupt->Halt();
	}
	else if(0 == currentThread->space->pinfo.nThreads){
		if(currentThread->space){
			currentThread->space->mark4Deletion = true;
			sysLock->Release();
			processesTable->Remove(currentThread->space->pinfo.spaceId);
			delete currentThread->space;
		}
		else{
			sysLock->Release();
		}
		currentThread->Finish();
	}
	else{
		sysCond->Wait(sysLock);
		currentThread->Finish();
		sysLock->Release();
	}

}
void Yield_Syscall(){
	currentThread->Yield();
}
#ifdef NETWORK
#define SERVER_PORT 0
#define MY_PORT 1
#define SERVER_MACHINE3 0
#endif

void Acquire_Syscall(unsigned int id){
#ifdef NETWORK
   	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];

	buffer[0] = 0;
	buffer[1] = 2;
	buffer[2] = id/10;
	buffer[3] = id%10;

    outPktHdr.to = SERVER_MACHINE3;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = MY_PORT;
    outMailHdr.length = 4 + 1;
	printf("client: acuire lock %d\n", id);
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, buffer);

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. \n");
    }
	memset(buffer, 0, MaxMailSize*sizeof(char));
    postOffice->Receive(MY_PORT, &inPktHdr, &inMailHdr, buffer);
    printf("Got packet from %d, box %d\n",inPktHdr.from,inMailHdr.from);
    printf("client: lock acuired %d\n", id);
    fflush(stdout);

#else
	Lock* lock;
	if ( (lock = (Lock *) currentThread->space->lockTable.Get(id)) ) {
	    lock->Acquire();
	    DEBUG('v', "acquire Lock: %s %d\n", lock->getName(), id);
	} else {
	    printf("%s","Bad id passed to Acquire\n");
	}
#endif
}
void Release_Syscall(unsigned int id){
#ifdef NETWORK
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];

	buffer[0] = 0;
	buffer[1] = 3;
	buffer[2] = id/10;
	buffer[3] = id%10;

    outPktHdr.to = SERVER_MACHINE3;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = MY_PORT;
    outMailHdr.length = 4 + 1;
	printf("client: release lock  %d\n", id);
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, buffer);

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running.\n");
    }
#else
	Lock* lock;
	if ( (lock = (Lock *) currentThread->space->lockTable.Get(id)) ) {
	    lock->Release();
	    DEBUG('v', "realease Lock: %s %d\n", lock->getName(), id);
	    if(lock->marked4Deletion && !lock->isWaiting()){
			delete lock;
		}
	} else {
	    printf("%s","Bad id passed to Release\n");
	}
#endif
}
int  CreateLock_Syscall(unsigned int vaddr, unsigned int len){
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    if (!buf) {
		printf("%s","Can't allocate kernel buffer in CreateLock\n");
		return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
		printf("%s","Bad pointer passed to CreateLock\n");
		delete[] buf;
		return -1;
    }
    buf[len]='\0';
#ifdef NETWORK
   	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];

	buffer[0] = 0;
	buffer[1] = 1;
	buffer[2] = len/10;
	buffer[3] = len%10;

	strcpy(buffer + 4, buf);

    outPktHdr.to = SERVER_MACHINE3;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = MY_PORT;
    outMailHdr.length = strlen(buf) + 4 + 1;
	printf("client: creat lock %s\n",buf);

    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, buffer);

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      return -1;
    }
	memset(buffer, 0, MaxMailSize*sizeof(char));
    postOffice->Receive(MY_PORT, &inPktHdr, &inMailHdr, buffer);
    printf("Got packet from %d, box %d\n",inPktHdr.from,inMailHdr.from);
    fflush(stdout);
    int id = buffer[0]/10 + buffer[1]%10;
    printf("client: created lock %s id %d\n",buf, id);
    return id;

#else
    Lock* lock = new Lock(buf);
	int id = 0;
    if ( lock ) {
		if ((id = currentThread->space->lockTable.Put(lock)) == -1 ){
		    delete lock;
		}
		DEBUG('v',"CREATE Lock: %s %d\n", buf, id);
		return id;
    }
    else{
		return -1;
	}
#endif
}
void DestroyLock_Syscall(unsigned int id){
#ifdef NETWORK
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];

	buffer[0] = 0;
	buffer[1] = 4;
	buffer[2] = id/10;
	buffer[3] = id%10;

    outPktHdr.to = SERVER_MACHINE3;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = MY_PORT;
    outMailHdr.length = 4 + 1;
	printf("client: destroy lock %d\n", id);
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, buffer);

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running.\n");
    }
#else
	Lock* lock;
	if ( (lock = (Lock *) currentThread->space->lockTable.Get(id)) ) {
		DEBUG('v', "Destroy LOCK: %s\n", lock->getName());
	    if(lock->isWaiting()){
			lock->marked4Deletion = true;
		}
		else{
			delete lock;
		}
	} else {
	    printf("%s","Bad id passed to DestroyLock\n");
	}
#endif
}
void Wait_Syscall(unsigned int id, unsigned int lockid){
#ifdef NETWORK
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];
	memset(buffer,0, MaxMailSize*sizeof(char));
	buffer[0] = 0;
	buffer[1] = 7;
	buffer[2] = id/10;
	buffer[3] = id%10;
	buffer[4] = lockid/10;
	buffer[5] = lockid%10;

    outPktHdr.to = SERVER_MACHINE3;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = MY_PORT;
    outMailHdr.length = 6 + 1;
	printf("client: wait condition %d lock %d\n", id, lockid);
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, buffer);

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running.\n");
    }
    memset(buffer, 0, MaxMailSize*sizeof(char));
	postOffice->Receive(MY_PORT, &inPktHdr, &inMailHdr, buffer);
	printf("Got packet from %d, box %d\n",inPktHdr.from,inMailHdr.from);
	printf("client: wait signalled %d lock %d\n", id, lockid);
	fflush(stdout);
	//////////////acquire lock////////////////
	memset(buffer,0, MaxMailSize*sizeof(char));
	buffer[0] = 0;
	buffer[1] = 2;
	buffer[2] = lockid/10;
	buffer[3] = lockid%10;

	outPktHdr.to = SERVER_MACHINE3;
	outMailHdr.to = SERVER_PORT;
	outMailHdr.from = MY_PORT;
	outMailHdr.length = 4 + 1;
	printf("client: in wait acuire lock %d\n", lockid);
	// Send the first message
	success = postOffice->Send(outPktHdr, outMailHdr, buffer);

	if ( !success ) {
	  printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
	}
	memset(buffer, 0, MaxMailSize*sizeof(char));
	postOffice->Receive(MY_PORT, &inPktHdr, &inMailHdr, buffer);
	printf("Got packet from %d, box %d\n",inPktHdr.from,inMailHdr.from);
	printf("client: in wait lock acuired %d\n", lockid);
	fflush(stdout);
	///////////////////////////////////////////
#else
	Lock* lock;
	if ( (lock = (Lock *) currentThread->space->lockTable.Get(lockid)) ) {
		Condition* cond;
		if ( (cond = (Condition *) currentThread->space->conditionTable.Get(id)) ) {
			DEBUG('v', "wait: %s %d\n", cond->getName(), id);
			cond->Wait(lock);

		} else {
			printf("%s","Bad id passed to Wait\n");
		}
	} else {
	    printf("%s","Bad lock id passed to Wait\n");
	}
#endif
}
void Signal_Syscall(unsigned int id, unsigned int lockid){
#ifdef NETWORK
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];

	buffer[0] = 0;
	buffer[1] = 8;
	buffer[2] = id/10;
	buffer[3] = id%10;
	buffer[4] = lockid/10;
	buffer[5] = lockid%10;

    outPktHdr.to = SERVER_MACHINE3;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = MY_PORT;
    outMailHdr.length = 6 + 1;
	printf("client: signal condition %d lock %d\n", id, lockid);
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, buffer);

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
    }

#else
	Lock* lock;
	if ( (lock = (Lock *) currentThread->space->lockTable.Get(lockid)) ) {
		Condition* cond;
		if ( (cond = (Condition *) currentThread->space->conditionTable.Get(id)) ) {
			cond->Signal(lock);
			DEBUG('v', "signal : %s %d\n", cond->getName(), id);
			if(cond->marked4Deletion && !cond->isWaiting()){
				delete cond;
			}
		} else {
			printf("%s","Bad id passed to Signal\n");
		}

	} else {
	    printf("%s","Bad lock id passed to Signal\n");
	}
#endif
}
void Broadcast_Syscall(unsigned int id, unsigned int lockid){
#ifdef NETWORK
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];

	buffer[0] = 0;
	buffer[1] = 9;
	buffer[2] = id/10;
	buffer[3] = id%10;
	buffer[4] = lockid/10;
	buffer[5] = lockid%10;

    outPktHdr.to = SERVER_MACHINE3;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = MY_PORT;
    outMailHdr.length = 6 + 1;
	printf("client: braodcast condition %d lock %d\n", id, lockid);
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, buffer);

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
    }
#else
	Lock* lock;
	if ( (lock = (Lock *) currentThread->space->lockTable.Get(lockid)) ) {
		Condition* cond;
		if ( (cond = (Condition *) currentThread->space->conditionTable.Get(id)) ) {
			cond->Broadcast(lock);
			DEBUG('v', "braodcaste : %s %d\n", cond->getName(), id);
			if(cond->marked4Deletion && !cond->isWaiting()){
				delete cond;
			}
		} else {
			printf("%s","Bad id passed to Broadcast\n");
		}
	} else {
	    printf("%s","Bad lock id passed to Broadcast\n");
	}
#endif
}
int  CreateCondition_Syscall(unsigned int vaddr, unsigned int len){
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    if (!buf) {
		printf("%s","Can't allocate kernel buffer in CreateCondition\n");
		return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
		printf("%s","Bad pointer passed to CreateCondition\n");
		delete[] buf;
		return -1;
    }
    buf[len]='\0';
#ifdef NETWORK
   	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];

	buffer[0] = 0;
	buffer[1] = 5;
	buffer[2] = len/10;
	buffer[3] = len%10;

	strcpy(buffer + 4, buf);

    outPktHdr.to = SERVER_MACHINE3;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = MY_PORT;
    outMailHdr.length = strlen(buf) + 4 + 1;
	printf("client: create condition %s \n", buf);
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, buffer);

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      return -1;
    }
    memset(buffer, 0, MaxMailSize*sizeof(char));
	postOffice->Receive(MY_PORT, &inPktHdr, &inMailHdr, buffer);
	printf("Got packet from %d, box %d\n",inPktHdr.from,inMailHdr.from);
    fflush(stdout);
    int id = buffer[2]/10 + buffer[3]%10;
    printf("client: condition created %d\n", id);
    return id;

#else
	DEBUG('t', "CREATE Cond: %s\n", buf);
    Condition* cond = new Condition(buf);
	int id = 0;
    if ( cond) {
		if ((id = currentThread->space->conditionTable.Put(cond)) == -1 ){
		    delete cond;
		}
		DEBUG('v', "CREATE Cond: %s %d\n", buf, id);
		return id;
    }
    else{
		return -1;
	}
#endif
}
void DestroyCondition_Syscall(unsigned int id){
#ifdef NETWORK
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];

	buffer[0] = 0;
	buffer[1] = 6;
	buffer[2] = id/10;
	buffer[3] = id%10;

    outPktHdr.to = SERVER_MACHINE3;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = MY_PORT;
    outMailHdr.length = 4 + 1;
	printf("client: destroy condition %d\n", id);
    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, buffer);

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
    }
#else
	Condition* cond;
	if ( (cond = (Condition *) currentThread->space->conditionTable.Get(id)) ) {
		DEBUG('v', "Destroy cond: %s\n", cond->getName());
	    if(cond->isWaiting()){
			cond->marked4Deletion = true;
		}
		else{
			delete cond;
		}
	} else {
	    printf("%s","Bad id passed to DestroyCondition\n");
	}
#endif
}
void PrintC_Syscall(unsigned int id){
	Condition* cond;
	if ( (cond = (Condition *) currentThread->space->conditionTable.Get(id)) ) {
		cond->printWaiting();
	} else {
	    printf("%s","Bad id passed to PrintC.\n");
	}
}
void PrintL_Syscall(unsigned int id){
	Lock* lock;
	if ( (lock = (Lock *) currentThread->space->lockTable.Get(id)) ) {
	    lock->printWaiting();
	} else {
	    printf("%s","Bad id passed to PrintL.\n");
	}
}
#ifdef NETWORK

#define SERVER_PORT 0
#define MY_PORT 1
#define SERVER_MACHINE 230

#define CUSTOMER 0
#define HELPER 1
#define OPERATOR 2

struct timeval tv;
struct timezone tz;
int sent = 0;
//////////////////////////////Customer system call////////////////////////////////////////////////
int CGetInLine_Syscall(){ // syscall type 0
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];
	memset(buffer, 0, MaxMailSize*sizeof(char));

	buffer[0] = CUSTOMER;
	buffer[1] = 0;

	gettimeofday(&tv, &tz);
	buffer[2] |= ((tv.tv_sec & 0xff000000) >> 24);
	buffer[3] |= ((tv.tv_sec & 0x00ff0000) >> 16);
	buffer[4] |= ((tv.tv_sec & 0x0000ff00) >> 8);
  	buffer[5] |= ((tv.tv_sec & 0x000000ff));

	buffer[6] |= ((tv.tv_usec & 0xff000000) >> 24);
	buffer[7] |= ((tv.tv_usec & 0x00ff0000) >> 16);
	buffer[8] |= ((tv.tv_usec & 0x0000ff00) >> 8);
  	buffer[9] |= ((tv.tv_usec & 0x000000ff));
  	buffer[10] = 0; //0 = client msg, serverId +1 = server msg

	//printf("sec %d, usec %d\n", tv.tv_sec, tv.tv_usec);
	sent = (sent+1)%nServer;
    outPktHdr.to = SERVER_MACHINE + sent;//Random()%nServer;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = currentThread->mailBoxNo;
    outMailHdr.length = 11 + 1;
	printf("sending packet to (%d, %d)\n", outPktHdr.to, outMailHdr.to);
    if ( !postOffice->Send(outPktHdr, outMailHdr, buffer)) {
      printf("The postOffice Send failed.\n");
    }
    memset(buffer, 0, MaxMailSize*sizeof(char));
	postOffice->Receive(currentThread->mailBoxNo, &inPktHdr, &inMailHdr, buffer);
	printf("Got packet from (%d, %d)\n",inPktHdr.from,inMailHdr.from);
    fflush(stdout);
    printf("Customer waiting in line %d\n", buffer[0]);
    return buffer[0];

}
void CWaitForHelper_Syscall(){
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];
	printf("Customer waiting for helper\n");
	memset(buffer, 0, MaxMailSize*sizeof(char));
	postOffice->Receive(currentThread->mailBoxNo, &inPktHdr, &inMailHdr, buffer);
	printf("Got packet from (%d, %d)\n",inPktHdr.from,inMailHdr.from);fflush(stdout);
}
void CWaitForUnload_Syscall(){// syscall type 1
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];
	/*memset(buffer, 0, MaxMailSize*sizeof(char));
	buffer[0] = CUSTOMER; //entity
	buffer[1] = 1; //type

    outPktHdr.to = SERVER_MACHINE;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = currentThread->mailBoxNo;
    outMailHdr.length = 2 + 1;
    // Send the first message
    if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
      printf("The postOffice Send failed.\n");
    }*/
    memset(buffer, 0, MaxMailSize*sizeof(char));
	postOffice->Receive(currentThread->mailBoxNo, &inPktHdr, &inMailHdr, buffer);
	printf("Got packet from (%d, %d)\n",inPktHdr.from,inMailHdr.from);
    fflush(stdout);
}
///////////////////////////////////helper////////////////////////////////////////////////////
void HRegister_Syscall(){
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];
	memset(buffer, 0, MaxMailSize*sizeof(char));

	buffer[0] = HELPER; //entity
	buffer[1] = 0; //type

	gettimeofday(&tv, &tz);
	buffer[2] |= ((tv.tv_sec & 0xff000000) >> 24);
	buffer[3] |= ((tv.tv_sec & 0x00ff0000) >> 16);
	buffer[4] |= ((tv.tv_sec & 0x0000ff00) >> 8);
  	buffer[5] |= ((tv.tv_sec & 0x000000ff));

	buffer[6] |= ((tv.tv_usec & 0xff000000) >> 24);
	buffer[7] |= ((tv.tv_usec & 0x00ff0000) >> 16);
	buffer[8] |= ((tv.tv_usec & 0x0000ff00) >> 8);
  	buffer[9] |= ((tv.tv_usec & 0x000000ff));
	buffer[10] = 0; //0 = client msg, 1 = server msg

	sent = (sent+1)%nServer;
    outPktHdr.to = SERVER_MACHINE + sent;//Random()%nServer;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = currentThread->mailBoxNo;
    outMailHdr.length = 11 + 1;
    printf("sending packet to (%d, %d)\n", outPktHdr.to, outMailHdr.to);
    if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
      printf("The postOffice Send failed.\n");
    }
    //memset(buffer, 0, MaxMailSize*sizeof(char));
	//postOffice->Receive(currentThread->mailBoxNo, &inPktHdr, &inMailHdr, buffer);
	//printf("Got packet from (%d, %d)\n",inPktHdr.from,inMailHdr.from);
    //fflush(stdout);
}

void HWaitForCar_Syscall(){
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];
	//printf("Helper waiting for car\n");
	memset(buffer, 0, MaxMailSize*sizeof(char));
	postOffice->Receive(currentThread->mailBoxNo, &inPktHdr, &inMailHdr, buffer);
	printf("Got packet from (%d, %d)\n",inPktHdr.from,inMailHdr.from);fflush(stdout);
}

int HSeatCustomer_Syscall(){
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];
	memset(buffer, 0, MaxMailSize*sizeof(char));
	buffer[0] = HELPER; //entity
	buffer[1] = 1; //type
	gettimeofday(&tv, &tz);
	buffer[2] |= ((tv.tv_sec & 0xff000000) >> 24);
	buffer[3] |= ((tv.tv_sec & 0x00ff0000) >> 16);
	buffer[4] |= ((tv.tv_sec & 0x0000ff00) >> 8);
  	buffer[5] |= ((tv.tv_sec & 0x000000ff));

	buffer[6] |= ((tv.tv_usec & 0xff000000) >> 24);
	buffer[7] |= ((tv.tv_usec & 0x00ff0000) >> 16);
	buffer[8] |= ((tv.tv_usec & 0x0000ff00) >> 8);
  	buffer[9] |= ((tv.tv_usec & 0x000000ff));
	buffer[10] = 0; //0 = client msg, 1 = server msg
	sent = (sent+1)%nServer;
    outPktHdr.to = SERVER_MACHINE + sent;//Random()%nServer;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = currentThread->mailBoxNo;
    outMailHdr.length = 11 + 1;
	printf("sending packet to (%d, %d)\n", outPktHdr.to, outMailHdr.to);
    if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
      printf("The postOffice Send failed.\n");
    }
    memset(buffer, 0, MaxMailSize*sizeof(char));
	postOffice->Receive(currentThread->mailBoxNo, &inPktHdr, &inMailHdr, buffer);
	printf("Got packet from (%d, %d)\n",inPktHdr.from,inMailHdr.from);
    fflush(stdout);
    //printf("Got %d\n", buffer[0]);
    return buffer[0];
}
//////////////////////////////////operator system call//////////////////////////////////////
void OStartRide_Syscall(){
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];
	memset(buffer, 0, MaxMailSize*sizeof(char));
	buffer[0] = OPERATOR; //entity
	buffer[1] = 0; //type
	gettimeofday(&tv, &tz);
	buffer[2] |= ((tv.tv_sec & 0xff000000) >> 24);
	buffer[3] |= ((tv.tv_sec & 0x00ff0000) >> 16);
	buffer[4] |= ((tv.tv_sec & 0x0000ff00) >> 8);
  	buffer[5] |= ((tv.tv_sec & 0x000000ff));

	buffer[6] |= ((tv.tv_usec & 0xff000000) >> 24);
	buffer[7] |= ((tv.tv_usec & 0x00ff0000) >> 16);
	buffer[8] |= ((tv.tv_usec & 0x0000ff00) >> 8);
  	buffer[9] |= ((tv.tv_usec & 0x000000ff));
	buffer[10] = 0; //0 = client msg, 1 = server msg

	sent = (sent+1)%nServer;
    outPktHdr.to = SERVER_MACHINE + sent;//Random()%nServer;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = currentThread->mailBoxNo;
    outMailHdr.length = 11 + 1;
    printf("sending packet to (%d, %d)\n", outPktHdr.to, outMailHdr.to);
    if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
      printf("The postOffice Send failed.\n");
    }
    memset(buffer, 0, MaxMailSize*sizeof(char));
	postOffice->Receive(currentThread->mailBoxNo, &inPktHdr, &inMailHdr, buffer);
	printf("Got packet from (%d, %d)\n",inPktHdr.from,inMailHdr.from);
    fflush(stdout);
}

void OFinishRide_Syscall(){
	PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];
	memset(buffer, 0, MaxMailSize*sizeof(char));
	buffer[0] = OPERATOR; //entity
	buffer[1] = 1; //type
	gettimeofday(&tv, &tz);
	buffer[2] |= ((tv.tv_sec & 0xff000000) >> 24);
	buffer[3] |= ((tv.tv_sec & 0x00ff0000) >> 16);
	buffer[4] |= ((tv.tv_sec & 0x0000ff00) >> 8);
  	buffer[5] |= ((tv.tv_sec & 0x000000ff));

	buffer[6] |= ((tv.tv_usec & 0xff000000) >> 24);
	buffer[7] |= ((tv.tv_usec & 0x00ff0000) >> 16);
	buffer[8] |= ((tv.tv_usec & 0x0000ff00) >> 8);
  	buffer[9] |= ((tv.tv_usec & 0x000000ff));

    buffer[10] = 0; //0 = client msg, 1 = server msg
	sent = (sent+1)%nServer;
    outPktHdr.to = SERVER_MACHINE + sent;//Random()%nServer;
    outMailHdr.to = SERVER_PORT;
    outMailHdr.from = currentThread->mailBoxNo;
    outMailHdr.length = 11 + 1;
	printf("sending packet to (%d, %d)\n", outPktHdr.to, outMailHdr.to);
    if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
      printf("The postOffice Send failed.\n");
    }
    memset(buffer, 0, MaxMailSize*sizeof(char));
	postOffice->Receive(currentThread->mailBoxNo, &inPktHdr, &inMailHdr, buffer);
	printf("Got packet from (%d, %d)\n",inPktHdr.from,inMailHdr.from);
    fflush(stdout);
}
#endif
#endif


void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2); // Which syscall?
    int rv=0; 	// the return value from a syscall
    if ( which == SyscallException ) {
		switch (type) {
			default:
			DEBUG('a', "Unknown syscall - shutting down.\n");
			case SC_Halt:
			DEBUG('a', "Shutdown, initiated by user program.\n");
			interrupt->Halt();
			break;
			case SC_Create:
			DEBUG('a', "Create syscall.\n");
			Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
			case SC_Open:
			DEBUG('a', "Open syscall.\n");
			rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
			case SC_Write:
			DEBUG('a', "Write syscall.\n");
			Write_Syscall(machine->ReadRegister(4),
					  machine->ReadRegister(5),
					  machine->ReadRegister(6));
			break;
			case SC_Read:
			DEBUG('a', "Read syscall.\n");
			rv = Read_Syscall(machine->ReadRegister(4),
					  machine->ReadRegister(5),
					  machine->ReadRegister(6));
			break;
			case SC_Close:
			DEBUG('a', "Close syscall.\n");
			Close_Syscall(machine->ReadRegister(4));
			break;
			case SC_Exec:
			DEBUG('a', "Exec syscall.\n");
			rv  = Exec_Syscall(machine->ReadRegister(4));
			break;
			case SC_Fork:
			DEBUG('a', "Fork syscall.\n");
			Fork_Syscall(machine->ReadRegister(4));
			break;

			case SC_Exit:
			DEBUG('a', "Exit syscall.\n");
			Exit_Syscall(machine->ReadRegister(4));
			break;
			case SC_Yield:
			DEBUG('a', "Yield syscall.\n");
			Yield_Syscall();
			break;

			case SC_Acquire:
			DEBUG('a', "Acquire syscall.\n");
			Acquire_Syscall(machine->ReadRegister(4));
			break;
			case SC_Release:
			DEBUG('a', "Release syscall.\n");
			Release_Syscall(machine->ReadRegister(4));
			break;
			case SC_CreateLock:
			DEBUG('a', "CreateLock syscall.\n");
			rv = CreateLock_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
			case SC_DestroyLock:
			DEBUG('a', "DestroyLock syscall.\n");
			DestroyLock_Syscall(machine->ReadRegister(4));
			break;
			case SC_Wait:
			DEBUG('a', "Wait syscall.\n");
			Wait_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
			case SC_Signal:
			DEBUG('a', "Signal syscall.\n");
			Signal_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
			case SC_Broadcast:
			DEBUG('a', "Broadcast syscall.\n");
			Broadcast_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
			case SC_CreateCondition:
			DEBUG('a', "CreateCondition syscall.\n");
			rv = CreateCondition_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
			case SC_DestroyCondition:
			DEBUG('a', "DestroyCondition syscall.\n");
			DestroyCondition_Syscall(machine->ReadRegister(4));
			break;
			case SC_PrintL:
			DEBUG('a', "DestroyCondition syscall.\n");
			PrintL_Syscall(machine->ReadRegister(4));
			break;
			case SC_PrintC:
			DEBUG('a', "DestroyCondition syscall.\n");
			PrintC_Syscall(machine->ReadRegister(4));
			break;
			case SC_CGetInLine:
			DEBUG('a',"CGetInLine syscall.\n");
			rv = CGetInLine_Syscall();
			break;
			case SC_CWaitForHelper:
			DEBUG('a',"CWaitForHelper syscall.\n");
			CWaitForHelper_Syscall();
			break;

			case SC_CWaitForUnload:
			DEBUG('a',"CWaitForUnload syscall.\n");
			CWaitForUnload_Syscall();
			break;

			case SC_OStartRide:
			DEBUG('a',"OStartRide  syscall.\n");
			OStartRide_Syscall();
			break;
			case SC_OFinishRide:
			DEBUG('a',"OFinishRide syscall.\n");
			OFinishRide_Syscall();
			break;
			case SC_HWaitForCar:
			DEBUG('a',"HWaitForCar syscall.\n");
			HWaitForCar_Syscall();
			break;
			case SC_HSeatCustomer:
			DEBUG('a',"HSeatCustomer syscall.\n");
			rv = HSeatCustomer_Syscall();
			break;
			case SC_HRegister:
			DEBUG('a',"HRegister syscall.\n");
			HRegister_Syscall();
			break;
		}

		// Put in the return value and increment the PC
		machine->WriteRegister(2,rv);
		machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
		machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
		machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
		return;
    }
#ifdef USE_TLB
#ifdef CHANGED
    else if ( which == PageFaultException ) {
		IntStatus oldLevel = interrupt->SetLevel(IntOff);

		int vpn = machine->ReadRegister(BadVAddrReg)/PageSize;
		int sid = currentThread->space->pinfo.spaceId;

		int ppn;
		for(ppn = 0; ppn <NumPhysPages; ++ppn){
			if(ipt[ppn].use == FALSE){
				continue;
			}
			if((ipt[ppn].virtualPage == vpn) && (ipt[ppn].spaceId == sid)){
				break;
			}
		}
		if(ppn == NumPhysPages){
			//the page is not in ITP
			ppn = IPTPages->Find();

			if(ppn == -1){
				//IPT is full evict a page from it.
				if(RAND_OR_FIFO == TRUE){
					ppn = fifoIptLocation;
				}
				else{
					ppn = Random() % NumPhysPages;
				}
				//don't evict the pages which are already in vpn
				bool done = FALSE;
				while(done != TRUE){
					done = TRUE;
					for(int i = 0; i < TLBSize; ++i){
						if(machine->tlb[i].physicalPage == ppn){
							done = FALSE;
							ppn = (ppn + 1)%NumPhysPages;
						}
					}
				}
				fifoIptLocation = (ppn + 1)%NumPhysPages; //FIFO
				AddrSpace *pagespace = (AddrSpace *)processesTable->Get(ipt[ppn].spaceId);
				ASSERT(pagespace->writeBack(ppn, ipt[ppn].virtualPage));
			}
			//set up IPT, pagetable
			ipt[ppn].virtualPage = vpn;
			ipt[ppn].use = TRUE;
			ipt[ppn].spaceId = sid;

			ASSERT(currentThread->space->readFrom(ppn, vpn));
		}
		//setup TLB
		machine->tlb[tblLocation].virtualPage = vpn;
		machine->tlb[tblLocation].physicalPage = ppn;
		machine->tlb[tblLocation].valid = TRUE;
		machine->tlb[tblLocation].use = FALSE;
		machine->tlb[tblLocation].dirty = FALSE;
		machine->tlb[tblLocation].readOnly = FALSE;//ipt[ppn].readOnly;

		DEBUG('v',"PageFaultException. vpn = %d, ppn = %d, spaceId = %d TLB = %d\n", vpn,ppn,sid, tblLocation );

		tblLocation = (tblLocation + 1)%TLBSize;
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
#endif
#endif
	else {
      cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
      interrupt->Halt();
    }
}
