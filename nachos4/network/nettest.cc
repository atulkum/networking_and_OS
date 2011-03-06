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

#define CUSTOMER 0
#define HELPER 1
#define OPERATOR 2
#define SERVER 4

#define MAX_CAR  13
#define LINES 4
#define SEATS_PER_LINE 2

class Message{
	public:
		int machineId;
		int port;
		time_t sec;  //second
		time_t usec; //micro second
		int type; //message type
		int entity; //0-operator, 1- helper, 2- customer, 3 -Safety Inspector
		int id;
};


//int seatedCount = 0;
int helperEnd;

List custInline[LINES];
List custSeated[LINES];
List helpers;
Message singleOpertor;

void processMsg(Message *msg){
	PacketHeader outPktHdr;
	MailHeader outMailHdr;
	char buffer[MaxMailSize];

	int minCount, minLine;
	ListElement *qf, *itr;
	Message *nextMsg;
	int i;

	switch(msg->entity){
		case CUSTOMER:
			printf("Got request from Customer of type %d\n", msg->type);

			switch(msg->type){
				case 0:
					minCount = custInline[0].size;
					minLine = 0;
					for(i = 1; i < LINES; ++i){
						if(custInline[i].size < minCount){
							minLine = i;
							minCount = custInline[i].size;
						}
					}
					custInline[minLine].Append(msg);
					//printf("size %d\n", custInline[minLine].size);
					if(msg->id == 0){
						memset(buffer, 0, MaxMailSize*sizeof(char));
						buffer[0] = minLine;
						outPktHdr.to = msg->machineId;
						outMailHdr.to = msg->port;
						outMailHdr.from = SERVER_PORT;
						outMailHdr.length = 1 + 1;
						printf("customer (%d, %d) waiting in line %d.\n", msg->machineId, msg->port, minLine);
						if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
				  			printf("The postOffice Send failed.\n");
						}
					}
					else{
						printf("customer waiting in line %d.\n", minLine);
					}


					break;
				//case 1:
					//printf("customer (%d, %d) seated in line %d.\n", msg->machineId, msg->port);
				//	break;
			}
			break;
		case HELPER:
			printf("Got request from Helper of type %d\n", msg->type);
			switch(msg->type){
				case 0:
					helpers.Append(msg);
					if(msg->id == 0){
						printf("helper (%d, %d) registered.\n", msg->machineId, msg->port);
					}
					else{
						printf("total %d helper registered.\n", helpers.size);
					}
				break;
				case 1:
					//get an empty seat
					for(i = 0;i < LINES; ++i){
						if(custSeated[i].size < SEATS_PER_LINE && !custInline[i].IsEmpty()){
							break;
						}
					}
					if(i != LINES){
						//assign a seat to the customer
						nextMsg = ( Message*)custInline[i].Remove();
						custSeated[i].Append(nextMsg);
						if(nextMsg->id == 0){
							//printf("helper (%d, %d) asign a seat to customer (%d,%d).\n",
							//	msg->machineId, msg->port,nextMsg->machineId, nextMsg->port);
							//tell the assigned customer that it is been seated.

							memset(buffer, 0, MaxMailSize*sizeof(char));
							buffer[0] = i; //just for testing
							outPktHdr.to = nextMsg->machineId;
							outMailHdr.to = nextMsg->port;
							outMailHdr.from = SERVER_PORT;
							outMailHdr.length = 1 + 1;
							printf("sit signal to customer (%d, %d)\n", nextMsg->machineId, nextMsg->port);
							if (!postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
								printf("The postOffice Send failed.\n");
							}
						}
						else{
							printf("sit signal to customer.\n");
						}
						if(msg->id == 0){
							//tell the helper that seat is assigned
							memset(buffer, 0, MaxMailSize*sizeof(char));
							buffer[0] = 0;
							outPktHdr.to = msg->machineId;
							outMailHdr.to = msg->port;
							outMailHdr.from = SERVER_PORT;
							outMailHdr.length = 1 + 1;
							printf("customer seated signal to helper (%d, %d)\n", msg->machineId, msg->port);
							if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
								printf("The postOffice Send failed.\n");
							}
						}
						else{
							printf("customer seated signal to helper.\n");
						}
					}
					else{
						helperEnd++;
						if(msg->id == 0){
							memset(buffer, 0, MaxMailSize*sizeof(char));
							buffer[0] = 1;
							outPktHdr.to = msg->machineId;
							outMailHdr.to = msg->port;
							outMailHdr.from = SERVER_PORT;
							outMailHdr.length = 1 + 1;
							printf("car loaded signal to helper (%d, %d)\n", msg->machineId, msg->port);
							if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
								printf("The postOffice Send failed.\n");
							}
						}
						else{
							printf("car loaded signal to helper.\n");
						}

						if(helperEnd == helpers.size){
							//tell operator to start the ride.
							if(singleOpertor.id == 0){
								memset(buffer, 0, MaxMailSize*sizeof(char));
								outPktHdr.to = singleOpertor.machineId;
								outMailHdr.to = singleOpertor.port;
								outMailHdr.from = SERVER_PORT;
								outMailHdr.length = strlen(buffer) + 1;
								printf("ready to go operator (%d, %d)\n", singleOpertor.machineId, singleOpertor.port);

								if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
								  printf("The postOffice Send failed.\n");
								}
							}
							else{
								printf("ready to go operator.\n");
							}
						}
					}
					delete msg;
				break;
			}
			break;
		case OPERATOR:
			printf("Got request from Operator of type %d\n", msg->type);
			switch(msg->type){
				case 0:
					if(msg->id == 0){
						printf("operator (%d, %d) registered.\n", msg->machineId, msg->port);
					}
					else{
						printf("operator registered.\n");
					}
					singleOpertor.machineId = msg->machineId;
					singleOpertor.port = msg->port;
					singleOpertor.type = msg->type;
					singleOpertor.entity = msg->entity;
					singleOpertor.sec = msg->sec;
					singleOpertor.usec = msg->usec;
					singleOpertor.id = msg->id;

					helperEnd = 0;
					qf = helpers.getFirst();
					itr = qf;
					//signal all helpers to load the car
					while(itr != NULL){
						nextMsg = ( Message*)itr->item;
						if(nextMsg->id == 0){
							memset(buffer, 0, MaxMailSize*sizeof(char));
							outPktHdr.to = nextMsg->machineId;
							outMailHdr.to = nextMsg->port;
							outMailHdr.from = SERVER_PORT;
							outMailHdr.length = strlen(buffer) + 1;
							printf("load signal to helper (%d, %d)\n", nextMsg->machineId, nextMsg->port);
							if (!postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
								printf("The postOffice Send failed.\n");
							}
						}
						else{
							printf("load signal to helper.\n");
						}
						itr = itr->next;
					}
					delete msg;
				break;
				case 1:
					//unload all the customer
					//if(msg->id == 0){
						for(i = 0;i < LINES; ++i){
							while(!custSeated[i].IsEmpty()){
								nextMsg = ( Message*)custSeated[i].Remove();
								if(nextMsg->id == 0){
									memset(buffer, 0, MaxMailSize*sizeof(char));
									outPktHdr.to = nextMsg->machineId;
									outMailHdr.to = nextMsg->port;
									outMailHdr.from = SERVER_PORT;
									outMailHdr.length = strlen(buffer) + 1;
									printf("unloading customer (%d, %d)\n", nextMsg->machineId, nextMsg->port);
									if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
									  printf("The postOffice Send failed.\n");
									}
								}
								else{
									printf("unloading customer.\n");
								}
							}
						}

						//send signal to operator that all customer unloaded
						if(singleOpertor.id == 0){
							memset(buffer, 0, MaxMailSize*sizeof(char));
							outPktHdr.to = singleOpertor.machineId;
							outMailHdr.to = singleOpertor.port;
							outMailHdr.from = SERVER_PORT;
							outMailHdr.length = strlen(buffer) + 1;
							printf("finish ride operator (%d, %d)\n", singleOpertor.machineId, singleOpertor.port);
							if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
								printf("The postOffice Send failed.\n");
							}
						}
						else{
							printf("finish ride operator.\n");
						}
					//}
					delete msg;
				break;
			}
			break;
	}
}
bool isBig(Message *msg1, Message *msg2){
	if(msg1->sec < msg2->sec) return true;
	else if(msg1->sec == msg2->sec){
		if(msg1->usec < msg2->usec) return true;
		else return false;
	}
	else return false;
}
void List::SortedInsertTimeStamp(void *item){
	Message *msg = (Message*)item;

   	ListElement *element = new ListElement(item, 0);
    ListElement *ptr;		// keep track
	size++;
    if (IsEmpty()) {	// if list is empty, put
        first = element;
        last = element;
        return;
    }

    if( isBig(msg, (Message*)first->item)) {
		element->next = first;
		first = element;
    } else {
        for (ptr = first; ptr->next != NULL; ptr = ptr->next) {
            if (isBig(msg , (Message*)ptr->next->item)) {
				element->next = ptr->next;
	    	    ptr->next = element;
				return;
	    	}
		}
		last->next = element;		// item goes at end of list
		last = element;
    }
}

void serverPart2(){
	PacketHeader outPktHdr, inPktHdr;
	MailHeader outMailHdr, inMailHdr;
	char buffer[MaxMailSize];
	Message *msg;
	List msgBuffer;
	time_t sec[5] = {0,0,0,0,0};
	time_t usec[5] = {0,0,0,0,0};

	printf("Total %d servers\n", nServer);
	while(true){
		printf("****************Server (%d,%d) waiting for packets.***************\n", serverId, netname);
		postOffice->Receive(SERVER_PORT, &inPktHdr, &inMailHdr, buffer);
		printf("Got packet from (%d, %d)\n",inPktHdr.from,inMailHdr.from);fflush(stdout);
		msg = new Message();
		msg->machineId = inPktHdr.from;
		msg->port = inMailHdr.from;
		msg->entity = buffer[0];
		msg->type = buffer[1];

		msg->sec = 0;
		msg->sec = (unsigned char)buffer[2];
		msg->sec = (msg->sec << 8)| (unsigned char)buffer[3];
		msg->sec = (msg->sec << 8)| (unsigned char)buffer[4];
		msg->sec = (msg->sec << 8)| (unsigned char)buffer[5];

		msg->usec = 0;
		msg->usec = (unsigned char)buffer[6];
		msg->usec = (msg->usec << 8)| (unsigned char)buffer[7];
		msg->usec = (msg->usec << 8)| (unsigned char)buffer[8];
		msg->usec = (msg->usec << 8)| (unsigned char)buffer[9];

		msg->id = buffer[10];

		int serStart = netname - serverId;
		if(msg->entity != SERVER){
			msgBuffer.SortedInsertTimeStamp(msg);
			//update it's time stamp, we are assuming there is no out of order packet
			//update only if timestamp is greater
			if(sec[serverId] < msg->sec){
				sec[serverId] = msg->sec;
				usec[serverId] = msg->usec;
			}
			else if(sec[serverId] == msg->sec){
				if(usec[serverId] < msg->usec){
					sec[serverId] = msg->sec;
					usec[serverId] = msg->usec;
				}
			}
			printf("timestamp (%ld, %ld) id %d\n", msg->sec, msg->usec, msg->id);
			if(msg->id == 0){
				buffer[10] = serverId + 1;
				//send msg to every server
				for(int s = 0; s < nServer; ++s){
					if(s == serverId) continue;
					outPktHdr.to = s + serStart;
					outMailHdr.to = SERVER_PORT;
					outMailHdr.from = SERVER_PORT;
					outMailHdr.length = 11 + 1;
					printf("send msg to server (%d, %d)\n", s + serStart, SERVER_PORT);
					if(!postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
						printf("The postOffice Send failed.\n");
					}
				}
			}
			else{
				if(sec[msg->id - 1] < msg->sec){
					sec[msg->id - 1] = msg->sec;
					usec[msg->id - 1] = msg->usec;
				}
				else if(sec[msg->id - 1] == msg->sec){
					if(usec[msg->id - 1] < msg->usec){
						sec[msg->id - 1] = msg->sec;
						usec[msg->id - 1] = msg->usec;
					}
				}
				//send ack back to every server with my time stamp
				for(int s = 0; s<nServer; ++s){
					if(s == serverId) continue;
					memset(buffer, 0, MaxMailSize*sizeof(char));

					buffer[0] = SERVER;
					buffer[1] = 0;

					buffer[2] |= ((sec[serverId]& 0xff000000) >> 24);
					buffer[3] |= ((sec[serverId] & 0x00ff0000) >> 16);
					buffer[4] |= ((sec[serverId] & 0x0000ff00) >> 8);
					buffer[5] |= ((sec[serverId] & 0x000000ff));

					buffer[6] |= ((usec[serverId] & 0xff000000) >> 24);
					buffer[7] |= ((usec[serverId] & 0x00ff0000) >> 16);
					buffer[8] |= ((usec[serverId] & 0x0000ff00) >> 8);
					buffer[9] |= ((usec[serverId] & 0x000000ff));

					buffer[10] = serverId + 1;

					outPktHdr.to = s + serStart;
					outMailHdr.to = SERVER_PORT;
					outMailHdr.from = SERVER_PORT;
					outMailHdr.length = 11 + 1;
					printf("send ack to server (%d, %d)\n", s + serStart, SERVER_PORT);
					if ( !postOffice->Send(outPktHdr, outMailHdr, buffer) ) {
						printf("The postOffice Send failed.\n");
					}
				}
			}
		}
		else{
			printf("Server rcvd an ack from (%d, %d) with timestamp (%ld, %ld).\n",
				msg->machineId, msg->port, msg->sec, msg->usec);
			//ack rcvd update timestamp
			if(sec[msg->id - 1] < msg->sec){
				sec[msg->id - 1] = msg->sec;
				usec[msg->id - 1] = msg->usec;
			}
			else if(sec[msg->id - 1] == msg->sec){
				if(usec[msg->id - 1] < msg->usec){
					sec[msg->id - 1] = msg->sec;
					usec[msg->id - 1] = msg->usec;
				}
			}
			delete msg;
		}
		printf("timestamp table (%ld, %ld),(%ld, %ld),(%ld, %ld),(%ld, %ld),(%ld, %ld)\n",
			sec[0],usec[0],sec[1],usec[1],sec[2],usec[2],sec[3],usec[3],sec[4],usec[4]);

		time_t minsec = sec[0];
		time_t minusec = usec[0];

		for(int i = 1; i < nServer; ++i){
			if(minsec > sec[i]){
				minsec = sec[i];
				minusec = usec[i];
			}
			else if(minsec == sec[i]){
				if(minusec > usec[i]){
					minsec = sec[i];
					minusec = usec[i];
				}
			}
		}
		printf("Min timestamp (%ld, %ld).\n",minsec, minusec);
		while(true){
			if(msgBuffer.IsEmpty()){
				break;
			}
			msg = (Message*)(msgBuffer.getFirst())->item;

			if(msg->sec < minsec){
				msg = (Message*)msgBuffer.Remove();
				processMsg(msg);
			}
			else if(msg->sec == minsec){
				if(msg->usec <= minusec){
					msg = (Message*)msgBuffer.Remove();
					processMsg(msg);
				}
				else{
					break;
				}
			}
			else{
				break;
			}
		}
	}
}
/*void serverPart1(){
	PacketHeader outPktHdr, inPktHdr;
	MailHeader outMailHdr, inMailHdr;
	char buffer[MaxMailSize];
	Message *msg;
	List msgBuffer;
	long int sec[5] = {0,0,0,0,0};
	long int usec[5] = {0,0,0,0,0};

	while(true){
	   	printf("****************Server waiting for packets.***************\n");
	   	postOffice->Receive(SERVER_PORT, &inPktHdr, &inMailHdr, buffer);
	   	printf("Got packet from (%d, %d)\n",inPktHdr.from,inMailHdr.from);fflush(stdout);
	   	msg = new Message();
	   	msg->machineId = inPktHdr.from;
	   	msg->port = inMailHdr.from;
		msg->entity = buffer[0];
		msg->type = buffer[1];

		msg->sec = 0;
		msg->sec = (unsigned char)buffer[2];
		msg->sec = (msg->sec << 8)| (unsigned char)buffer[3];
	    msg->sec = (msg->sec << 8)| (unsigned char)buffer[4];
  		msg->sec = (msg->sec << 8)| (unsigned char)buffer[5];

		msg->usec = 0;
		msg->usec = (unsigned char)buffer[6];
		msg->usec = (msg->usec << 8)| (unsigned char)buffer[7];
	    msg->usec = (msg->usec << 8)| (unsigned char)buffer[8];
  		msg->usec = (msg->usec << 8)| (unsigned char)buffer[9];

		printf("sec %d, usec %d\n", msg->sec, msg->usec);
		processMsg(msg);
	}
}*/