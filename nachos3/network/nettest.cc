// nettest.cc
//	Test out message delivery between two "Nachos" machines,
//	using the Post Office to coordinate delivery.
//
//	Two caveats:
//	  1. Two copies of Nachos must be running, with machine ID's 0 and 1:
//		./nachos -m 0 -o 1 &
//		./nachos -m 1 -o 0 &
//
//	  2. You need an implementation of condition variables,
//	     which is *not* provided as part of the baseline threads
//	     implementation.  The Post Office won't work without
//	     a correct implementation of condition variables.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"

// Test out message delivery, by doing the following:
//	1. send a message to the machine with ID "farAddr", at mail box #0
//	2. wait for the other machine's message to arrive (in our mailbox #0)
//	3. send an acknowledgment for the other machine's message
//	4. wait for an acknowledgement from the other machine to our
//	    original message

void
MailTest(int farAddr)
{
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = "Hello there!";
    char *ack = "Got it!";
    char buffer[MaxMailSize];

    // construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
    outPktHdr.to = farAddr;
    outMailHdr.to = 0;
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data);

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the first message from the other machine
    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Send acknowledgement to the other machine (using "reply to" mailbox
    // in the message that just arrived
    outPktHdr.to = inPktHdr.from;
    outMailHdr.to = inMailHdr.from;
    outMailHdr.length = strlen(ack) + 1;
    success = postOffice->Send(outPktHdr, outMailHdr, ack);

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the ack from the other machine to the first message we sent.
    postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Then we're done!
    interrupt->Halt();
}

#define SERVER_PORT 0

typedef struct distributedLock{
	char name[256];
	int queue[256][2];
	int queueSize;
	bool marked4Deletion;
	int owner;
}DistributedLock;

DistributedLock locks[256];
BitMap lockIds(256);

typedef struct distributedCondition{
	char name[256];
	int queue[256][2];
	int queueSize;
	bool marked4Deletion;
	int lock;
}DistributedCondition;

DistributedCondition conditions[256];
BitMap conditionIds(256);

void server(){
	PacketHeader outPktHdr, inPktHdr;
	MailHeader outMailHdr, inMailHdr;
	char buffer[MaxMailSize];
	bool success;
	char name[256];
	int type, id, lockid, msgLen;

	outMailHdr.from = SERVER_PORT;
	while(true){
	   	printf("****************Server waiting for packets.***************\n");
	   	postOffice->Receive(SERVER_PORT, &inPktHdr, &inMailHdr, buffer);
	   	printf("Got packet from %d, box %d\n",inPktHdr.from,inMailHdr.from);
	    fflush(stdout);
		type = buffer[0]*10 + buffer[1];
		printf("request type %d\n", type);
		switch(type){
			case 1://create lock
				printf("server:create lock %s \n",buffer+4);
				id = -1;
				msgLen = buffer[2]*10 + buffer[3];
				//printf("server: msgLen %d\n", msgLen);
				memset(name,0, 256*sizeof(char));
				memcpy(name, buffer+4, msgLen);
				for(int i = 0; i < 256; ++i){
					if(lockIds.Test(i)){
						if(!strcmp(name, locks[i].name)){
							id = i;
							break;
						}
					}
				}
				if(id == -1){
					id=lockIds.Find();
					ASSERT(id != -1);
					memset(locks[id].name,0, 256*sizeof(char));
					memcpy(locks[id].name, name, msgLen);
					locks[id].marked4Deletion = false;
					locks[id].owner = -1;
					locks[id].queueSize = 0;
				}
				memset(buffer,0, MaxMailSize*sizeof(char));
				buffer[0] = id/10; buffer[1] = id%10;

			    outPktHdr.to = inPktHdr.from;
			    outMailHdr.to = inMailHdr.from;
			    outMailHdr.length = 2 + 1;
			    printf("server: lock created %d %s \n", id, locks[id].name);
			    success = postOffice->Send(outPktHdr, outMailHdr, buffer);

			    if ( !success ) {
			      printf("The postOffice Send failed. You must not have the other Nachos running.\n");
			    }
			break;
			case 2://Acquire
				id = buffer[2]*10 + buffer[3];
				printf("server: acquire lock %d\n", id);
				if(locks[id].owner == inPktHdr.from){
					printf("ERROR: Lock %d already held by the client %d.\n", id, inPktHdr.from);
				}
				else if(locks[id].owner == -1){
					locks[id].owner = inPktHdr.from;
					memset(buffer,0, MaxMailSize*sizeof(char));

				    outPktHdr.to = inPktHdr.from;
				    outMailHdr.to = inMailHdr.from;
				    outMailHdr.length = strlen(buffer) + 1;
				    printf("server:lock acuired.%d\n", id);
				    success = postOffice->Send(outPktHdr, outMailHdr, buffer);

				    if ( !success ) {
				      printf("The postOffice Send failed. You must not have the other Nachos running.\n");
				    }
				}
				else{
					locks[id].queue[locks[id].queueSize][0] = inPktHdr.from;
					locks[id].queue[locks[id].queueSize][1] = inMailHdr.from;
					locks[id].queueSize++;
				}
			break;
			case 3://Release
				id = buffer[2]*10 + buffer[3];
				printf("server: release lock %d\n", id);
				if(locks[id].owner != inPktHdr.from){
					printf("ERROR: Lock %d is not held by the client %d.\n", id, locks[id].owner);//inPktHdr.from);
				}
				else if(locks[id].queueSize > 0){
					printf("server:lock acuired.%d\n", id);
					locks[id].queueSize--;
					locks[id].owner = locks[id].queue[locks[id].queueSize][0];
					memset(buffer,0, MaxMailSize*sizeof(char));
				    outPktHdr.to = locks[id].queue[locks[id].queueSize][0];
				    outMailHdr.to = locks[id].queue[locks[id].queueSize][1];
				    outMailHdr.length = strlen(buffer) + 1;
				    success = postOffice->Send(outPktHdr, outMailHdr, buffer);

				    if ( !success ) {
				      printf("The postOffice Send failed. You must not have the other Nachos running.\n");
				    }
				}
				else {
					locks[id].owner = -1;
					if(locks[id].marked4Deletion){
						lockIds.Clear(id);
					}
				}
			break;
			case 4://destroy
				id = buffer[2]*10 + buffer[3];
				printf("server: destroy lock %d\n", id);
				if(locks[id].queueSize > 0){
					locks[id].marked4Deletion = true;
				}
				else {
					lockIds.Clear(id);
				}
			break;
			case 5: // create
				printf("server:create condition %s \n",buffer+4);
				id = -1;
				msgLen = buffer[2]*10 + buffer[3];
				//printf("server: msgLen %d\n", msgLen);
				memset(name,0, 256*sizeof(char));
				memcpy(name, buffer+4, msgLen);
				for(int i = 0; i < 256; ++i){
					if(conditionIds.Test(i)){
						if(!strcmp(name, conditions[i].name)){
							id = i;
							break;
						}
					}
				}
				if(id == -1){
					id=conditionIds.Find();
					ASSERT(id != -1);
					memset(conditions[id].name,0, 256*sizeof(char));
					memcpy(conditions[id].name, name, msgLen);
					conditions[id].marked4Deletion = false;
					conditions[id].lock = -1;
					conditions[id].queueSize = 0;
				}
				memset(buffer,0, MaxMailSize*sizeof(char));
				buffer[0] = id/10; buffer[1] = id%10;

			    outPktHdr.to = inPktHdr.from;
			    outMailHdr.to = inMailHdr.from;
			    outMailHdr.length = 2 + 1;
			    printf("server: condition created %d %s \n", id, conditions[id].name);
			    success = postOffice->Send(outPktHdr, outMailHdr, buffer);

			    if ( !success ) {
			      printf("The postOffice Send failed. You must not have the other Nachos running. \n");
			    }
			break;
			case 6: // destroy
				id = buffer[2]*10 + buffer[3];
				printf("server: destroy condition %d\n", id);
				/*if(!conditionIds.Test(id)){
					printf("ERROR: Condition %d does not exists.\n", id);
					break;
				}*/
				if(conditions[id].queueSize > 0){
					conditions[id].marked4Deletion = true;
				}
				else {
					conditionIds.Clear(id);
				}
			break;
			case 7: // wait
				id = buffer[2]*10 + buffer[3];
				lockid = buffer[4]*10 + buffer[5];
				printf("server: wait on condition %d with lock %d\n", id, lockid);
				if(locks[lockid].owner != inPktHdr.from){
					printf("ERROR: Lock %d is not held by the client %d.\n", lockid, inPktHdr.from);
					break;
				}
				else if(conditions[id].lock == -1){
					conditions[id].lock = lockid;
				}
				else if(conditions[id].lock != lockid){
					printf("ERROR: Condition %d doesn't hold the lock %d.\n",id, lockid);
					break;
				}
				//////////////release the lock///////////////////
				if(locks[lockid].queueSize > 0){
					printf("server:in wait lock acuired.%d\n", id);
					locks[lockid].queueSize--;
					locks[lockid].owner = locks[lockid].queue[locks[lockid].queueSize][0];
					memset(buffer,0, MaxMailSize*sizeof(char));
				    outPktHdr.to = locks[lockid].queue[locks[lockid].queueSize][0];
				    outMailHdr.to = locks[lockid].queue[locks[lockid].queueSize][1];
				    outMailHdr.length = strlen(buffer) + 1;
				    success = postOffice->Send(outPktHdr, outMailHdr, buffer);
				    if ( !success ) {
				      printf("The postOffice Send failed. You must not have the other Nachos running.\n");
				    }
				}
				else {
					locks[lockid].owner = -1;
					if(locks[lockid].marked4Deletion){
						lockIds.Clear(lockid);
					}
				}
				//////////////////////////////////////////////////
				conditions[id].queue[conditions[id].queueSize][0] = inPktHdr.from;
				conditions[id].queue[conditions[id].queueSize][1] = inMailHdr.from;
				conditions[id].queueSize++;

			break;
			case 8: // signal
				id = buffer[2]*10 + buffer[3];
				lockid = buffer[4]*10 + buffer[5];
				printf("server: signal on condition %d with lock %d\n", id, lockid);

				if(conditions[id].queueSize > 0){
					if(locks[lockid].owner != inPktHdr.from){
						printf("ERROR: Lock %d is not held by the client %d.\n", lockid, inPktHdr.from);
					}
					else if(conditions[id].lock != lockid){
						printf("ERROR: Condition %d is not waiting on lock %d.\n",id, lockid);
					}
					else{
						conditions[id].queueSize--;
						memset(buffer,0, MaxMailSize*sizeof(char));
					    outPktHdr.to = conditions[id].queue[conditions[id].queueSize][0];
					    outMailHdr.to = conditions[id].queue[conditions[id].queueSize][1];
					    outMailHdr.length = strlen(buffer) + 1;
					    success = postOffice->Send(outPktHdr, outMailHdr, buffer);
						printf("server: signalled %d\n", conditions[id].queue[conditions[id].queueSize][0]);
					    if ( !success ) {
					      printf("The postOffice Send failed. You must not have the other Nachos running.\n");
					    }
					}
					if(conditions[id].queueSize == 0){
						conditions[id].lock = -1;
						if(conditions[id].marked4Deletion){
							conditionIds.Clear(id);
						}
					}
				}
			break;
			case 9: //broadcast
				id = buffer[2]*10 + buffer[3];
				lockid = buffer[4]*10 + buffer[5];
				printf("server: signal on condition %d with lock %d\n", id, lockid);
				/*if(!conditionIds.Test(id)){
					printf("ERROR: Condition %d does not exists.\n", id);
					break;
				}*/
				if(conditions[id].queueSize > 0){
					if(locks[lockid].owner != inPktHdr.from){
						printf("ERROR: Lock %d is not held by the client %d.\n", lockid, inPktHdr.from);
					}
					else if(conditions[id].lock != lockid){
						printf("ERROR: Condition %d doesn't hold the lock %d.\n",id, lockid);
					}
					else{
						while(conditions[id].queueSize > 0){
							conditions[id].queueSize--;
							memset(buffer,0, MaxMailSize*sizeof(char));
					    	outPktHdr.to = conditions[id].queue[conditions[id].queueSize][0];
					    	outMailHdr.to = conditions[id].queue[conditions[id].queueSize][1];
					    	outMailHdr.length = strlen(buffer) + 1;
					    	success = postOffice->Send(outPktHdr, outMailHdr, buffer);

					    	if ( !success ) {
					    	  printf("The postOffice Send failed. You must not have the other Nachos running.\n");
					    	}
						}
						conditions[id].lock = -1;
						if(conditions[id].marked4Deletion){
							conditionIds.Clear(id);
						}
					}
				}
			break;
			default: break;
		}
	}
}