#include "mm2.h"

double lambda = (double)0.5; // default value 0.5
double mu = (double)0.35; //default value 035
int isSingleServer = 0; //0 = double, 1 = single
long seedval = 0; //default value 0
unsigned int qsize = 5;//default value 5
unsigned int nCustomer = 20; //default value 20
int distr = DISTR_EXP; //default value exponential distribution
int isTraceFile = 0; //default value MM2


//head and tail of the queue
ListElement *qhead = NULL;
ListElement *qtail = NULL;
//length of the queue
int qlen = 0;

//starting time
struct timeval startTime;
struct timezone tz;
//server and sigint handler thread ids
pthread_t server1Thread;
pthread_t server2Thread;
pthread_t sigintThread;
//lock for printing
pthread_mutex_t printLock;
//lock for accessing queue
pthread_mutex_t qLock;
//condition variable for signalling that queue is full
pthread_cond_t qFilled;
//condition variable and lock for signalling an interrupt
pthread_cond_t intCond;
pthread_mutex_t intLock;
//signal handler mask set
sigset_t newsig;
//global falgs to indicate finishing
//finish=1, means arrival thread is done
//finishReq=1, means an interrupt has occured
int finish = 0;
int finishReq = 0;
//trace file object and filename
FILE *tracefile;
char filename[512+1];

///////stats/////////////////
//number of dropped customer
unsigned int ndrop = 0;
//total sum of inter arriavl time
long double totalInterArrTime = 0;
//total customer arrivaed
unsigned int ntotalInterArrTime = 0;
//total sum of service time
long double totalSerTime[2];
//total customer serviced
unsigned int ntotalSerTime[2];
//total time customers spend in quque
long double custinQ = 0;
//total system time
long double sysTime = 0;
long double sysTime1 = 0;
//total customer arriaved in system
unsigned int nsysTime = 0;
//sum of square of system time, used in standard deviation calculation
long double sysTime_sqr = 0;
//////////////////////////////
int main(int argc, char **argv){
	//parse the command line one by one
	if(ProcessOptions(argc, argv) == -1){
		fprintf(stderr, "Error: command line parsing fails.\n");
		pthread_exit(NULL);
	}
	//initialize
	if(init()== -1){
		fprintf(stderr, "Error: init fails.\n");
		pthread_exit(NULL);
	}
	char printmsg[256];

	//previous arrival time
	struct timeval preArrTime;

	struct timeval temptv;
	struct timeval delay;

	totalSerTime[0] = 0;
	totalSerTime[1] = 0;

	ntotalSerTime[0] = 0;
	ntotalSerTime[1] = 0;
	//intially set previous arriaval time to start time
	preArrTime.tv_sec = startTime.tv_sec;
	preArrTime.tv_usec = startTime.tv_usec;
	//inter arriaval and service time for a customer
	int arrtime;
	int sertime;
	char line[80];

	int i = 0;
	for(; i < nCustomer; ++i){
		////////////////check for finish due to interrupt handler///////////////////////////
		if(finishReq){
			break;
		}

		/////////sleep for inter arrival time//////////////////////////
		//get the next arrival time
		if(isTraceFile){
			//read from file
			memset(line, 0, 80);
			if(NULL == fgets(line, 80, tracefile)){
				//in case of error exit
				printf("ERROR: tracefile read fails\n");
				destroy();
				pthread_exit(NULL);
			}
			//trim the blank space
			TrimBlanks(line);
			//sscanf (line, "%d %d", &arrtime, &sertime);// in case of error exit
			//read the arrival and service time
			readArrSer(line, &arrtime, &sertime);

			//printf(">>>>>>>>>>>arrtime %d, sertime %d\n", arrtime,sertime);
		}
		else{
			//else generate from the distribution
			arrtime = GetInterval(distr, lambda);// in millis
			sertime = GetInterval(distr, mu);
		}
		//get the current time
		memset(&temptv, 0, sizeof(struct timeval));
		gettimeofday(&temptv, &tz);
		//sleep for [next arrival time - (current time - pre arrival time)]
		timeElapsed(&preArrTime, &temptv, &temptv);

		delay.tv_sec = arrtime/1000 - temptv.tv_sec ;
		delay.tv_usec = (arrtime%1000)*1000 - temptv.tv_usec;

		if (delay.tv_usec < 0){
			delay.tv_sec--;
			delay.tv_usec += 1000000;
		}

		if(delay.tv_sec > 0 || (delay.tv_sec == 0 && delay.tv_usec > 0)){
			select(0, NULL, NULL, NULL, &delay);
			if(finishReq){
				break;
			}
		}
		//////////////new customer arrives//////////////////////////
		Customer* cust = (Customer*) malloc(sizeof(Customer));
		if(cust == NULL){
			printf("ERROR: malloc fails\n");
			destroy();
			pthread_exit(NULL);
		}
		memset(cust, 0, sizeof(Customer));
		//get the arrival time
		gettimeofday(&(cust->a), &tz);
		//set the id
		cust->id = i + 1;
        //set the service time, it will be used in server thread
		cust->sertime = sertime;
		//calculate and print the inter arrival time between two customers
		timeElapsed(&preArrTime, &(cust->a), &temptv);

		memset(printmsg, 0, 256);
		sprintf(printmsg, "c%d arrives, inter-arrival time = %d.%03dms\n",
			cust->id, (int)(temptv.tv_sec*1000 + temptv.tv_usec/1000), (int)temptv.tv_usec%1000);
		//store in the running sum the inter arrival time
		totalInterArrTime += temptv.tv_sec*1000000 + temptv.tv_usec;

		ntotalInterArrTime++;

		printtimemsg(&(cust->a), printmsg);
		//update the previous arrival time
		preArrTime.tv_sec = cust->a.tv_sec;
		preArrTime.tv_usec = cust->a.tv_usec;
		///////////////////////////////////////////////////////////
		//put in the queue
		//get the queue lock
		pthread_mutex_lock(&qLock);
		//chaeck if there is space in queue
		if(qlen < qsize){
			Append(&qhead, &qtail, cust);
			qlen++;
			gettimeofday(&(cust->qs), &tz);

			memset(printmsg, 0, 256);
			sprintf(printmsg, "c%d enters Q1\n", cust->id);
			printtimemsg(&(cust->qs), printmsg);

			//signal the servers
			pthread_cond_broadcast(&qFilled);
			pthread_mutex_unlock (&qLock);
		}
		else{
			//queue full drop the customer
			pthread_mutex_unlock (&qLock);
			ndrop++;
			memset(printmsg, 0, 256);
			sprintf(printmsg, "c%d dropped\n", cust->id);
			printtimemsg(&(cust->a), printmsg);
		}
	}
	/////////////////finish all customer served or an interrupt occur////////////////////////////
	pthread_mutex_lock (&qLock);
	finish = 1;
	pthread_cond_broadcast(&qFilled);
	pthread_mutex_unlock (&qLock);

	pthread_cond_signal(&intCond);
	/////////////////wait for the servers to finish//////////////
	void *status;
	pthread_join(server1Thread, &status);
	if(!isSingleServer){
		pthread_join(server2Thread, &status);
	}

	memset(&temptv, 0, sizeof(struct timeval));
	gettimeofday(&temptv, &tz);
	memset(printmsg, 0, 256);
	//sprintf(printmsg, "emulation ends\n");
	//printtimemsg(&temptv, printmsg);
	/////////////////////print statistics////////////////////////
pthread_mutex_lock (&printLock);
if(ntotalInterArrTime != 0){
	//temptv contains total emulation time
	timeElapsed(&startTime, &temptv, &temptv);

	long double emulationTime = temptv.tv_sec*1000000 + temptv.tv_usec;

	printf("\n  Statistics:\n\n");

	long double tempTime;

	tempTime = totalInterArrTime/ntotalInterArrTime;

	printf("    average inter-arrival time = %d.%03d seconds\n", round551(tempTime)/1000000, round551(tempTime)%1000000);
	//round551(tempTime)/1000, round551(tempTime)%1000);
	if(ntotalSerTime[0] + ntotalSerTime[1] > 0){
		tempTime = totalSerTime[0] + totalSerTime[1];
		tempTime = tempTime/(ntotalSerTime[0] + ntotalSerTime[1]);
		printf("    average service time = %d.%03d seconds\n\n", round551(tempTime)/1000000, round551(tempTime)%1000000);
	}
	else{
		printf("    average service time = insufficient data\n\n");
	}

	tempTime = custinQ/emulationTime;
	printf("    average number of customers in Q1 = %.6Lg\n", tempTime);

	tempTime = totalSerTime[0]/emulationTime;
	printf("    average number of customers at S1 = %.6Lg\n", tempTime);

	tempTime = totalSerTime[1]/emulationTime;
	printf("    average number of customers at S2 = %.6Lg\n\n", tempTime);
	if(nsysTime > 0){
		tempTime = sysTime/nsysTime;
		printf("    average time spent in system = %d.%03d seconds\n", round551(tempTime)/1000000, round551(tempTime)%1000000);

		tempTime = sysTime1/nsysTime;
		tempTime = sqrt(sysTime_sqr/nsysTime - tempTime*tempTime);
		printf("    standard deviation for time spent in system = %d.%03d seconds\n", round551(tempTime)/1000, round551(tempTime)%1000);
	}
	else{
		printf("    average time spent in system = insufficient data\n");
		printf("    standard deviation for time spent in system = insufficient data\n");
	}
	tempTime = (double)ndrop/(double)ntotalInterArrTime;
	printf("    customer drop probability = %.6Lg\n\n", tempTime);
}
	/////////////////////////////////////////////////////////
pthread_mutex_unlock (&printLock);
	destroy();
	pthread_exit(NULL);
}
//methos for server thread
void* serverThreadHandler(void *serverid){

	/*struct sigaction act;
	act.sa_handler = dummyInterrupt;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);*/

	pthread_sigmask(SIG_BLOCK, &newsig, NULL);

	int id = (int)serverid;
	int done = 0;
	//wait for the customer arrival in infinite loop
	while(1){
		//check the queue for the customer
		//acuire the lock
		pthread_mutex_lock(&qLock);
		while(qlen == 0){
			//if finished and queue is empty shut down the server
			if(finish){
				done = 1;
				break;
			}
			//wait for the queue fill up
			pthread_cond_wait(&qFilled, &qLock);
		}
		//if finished and queue is empty shut down the server
		if(done) {
			pthread_mutex_unlock(&qLock);
			break;
		}
		// get the customer from the queue
		Customer* newCust = (Customer*)Remove(&qhead, &qtail);
		qlen--;
		//set the out of the queue time
		gettimeofday(&(newCust->qf), &tz);
		pthread_mutex_unlock (&qLock);
		// calculate the time in queue and print
		struct timeval temptv;
		timeElapsed(&(newCust->qs), &(newCust->qf), &temptv);

		char printmsg[256];
		memset(printmsg, 0, 256);
		sprintf(printmsg, "c%d leaves Q1, time in Q1 = %d.%03dms\n", newCust->id,
			(int)(temptv.tv_sec*1000 + temptv.tv_usec/1000), (int)temptv.tv_usec%1000);
		printtimemsg(&(newCust->qf), printmsg);
		//update the running sum
		custinQ += temptv.tv_sec*1000000 + temptv.tv_usec;

		//if interrupt has not been occur serve the customer
		if(!finishReq){
			int sertime = newCust->sertime;
			struct timeval delay;
			delay.tv_sec = sertime/1000;
			delay.tv_usec = (sertime%1000)*1000;

			memset(printmsg, 0, 256);
			sprintf(printmsg, "c%d begin service at S%d\n", newCust->id, id);

			gettimeofday(&(newCust->s), &tz);
			printtimemsg(&(newCust->s), printmsg);
			//sleep till service time has done
			select(0, NULL, NULL, NULL, &delay);
			//update and print the depart time
			gettimeofday(&(newCust->d), &tz);

			timeElapsed(&(newCust->s), &(newCust->d), &temptv);

			struct timeval temptv1;
			timeElapsed(&(newCust->a), &(newCust->d), &temptv1);

			memset(printmsg, 0, 256);
			sprintf(printmsg, "c%d departs from S%d, service time = %d.%03dms time in system = %d.%03dms\n",
				newCust->id, id,
				(int)(temptv.tv_sec*1000 + temptv.tv_usec/1000), (int)temptv.tv_usec%1000,
				(int)(temptv1.tv_sec*1000 + temptv1.tv_usec/1000), (int)temptv1.tv_usec%1000);
			printtimemsg(&(newCust->d), printmsg);
			//update the running sum
			totalSerTime[id-1] += temptv.tv_sec*1000000 + temptv.tv_usec;

			ntotalSerTime[id-1]++;

			sysTime += temptv1.tv_sec*1000000 + temptv1.tv_usec;

			nsysTime++;
			sysTime1 = temptv1.tv_sec * 1000 + round551(temptv1.tv_usec/1000.0);
			sysTime_sqr += (temptv1.tv_sec * 1000 + round551(temptv1.tv_usec/1000.0))*(temptv1.tv_sec * 1000 + round551(temptv1.tv_usec/1000.0));
		}
		else{
			//if interrupt has occur , get the customer out of queue and instead of serving them,
			//depart them out of the queue and calculate and print the depart time
			gettimeofday(&(newCust->d), &tz);
			timeElapsed(&(newCust->a), &(newCust->d), &temptv);
			memset(printmsg, 0, 256);
			sprintf(printmsg, "c%d departs from S%d, service time = %d.%03dms time in system = %d.%03dms\n",
				newCust->id, id, 0, 0,
				(int)(temptv.tv_sec*1000 + temptv.tv_usec/1000), (int)temptv.tv_usec%1000);
			printtimemsg(&(newCust->d), printmsg);
			// update the running sum
			sysTime += temptv.tv_sec*1000000 + temptv.tv_usec;
			nsysTime++;
			sysTime1 = temptv.tv_sec * 1000 + round551(temptv.tv_usec/1000.0);
			sysTime_sqr += (temptv.tv_sec * 1000 + round551(temptv.tv_usec/1000.0))*(temptv.tv_sec * 1000 + round551(temptv.tv_usec/1000.0));

		}
		//release the memory
		free(newCust);
	}
	pthread_exit(NULL);
}
//interrupt thread handler, wait for the signal which is sent when an interrupt occur
void* interruptThreadHandler(void *data){
	struct sigaction act;
	act.sa_handler = interrupt;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	pthread_sigmask(SIG_UNBLOCK, &newsig, NULL);
	//wait for the interrupted condition
	pthread_mutex_lock (&intLock);
	pthread_cond_wait(&intCond, &intLock);
	pthread_mutex_unlock (&intLock);
	pthread_exit(NULL);
}
//interrupt handler routine, send signal when interrupt has occur
void interrupt(int sig){
	printf("SIGINT signal caught.\n");
	finishReq = 1;
	pthread_cond_signal(&intCond);
}
//no op interrupt routine
void dummyInterrupt(int sig){

}
//a method to calculate the elapsed time
//return diff = tv2 - tv1
void timeElapsed(struct timeval* tv1, struct timeval* tv2, struct timeval* diff){
	diff->tv_sec = tv2->tv_sec - tv1->tv_sec;
	diff->tv_usec = tv2->tv_usec - tv1->tv_usec;
	if (diff->tv_usec < 0){
        diff->tv_sec--;
        diff->tv_usec += 1000000;
	}
}
//initialization routine
int init(){
	//fill the signal mask
	sigemptyset(&newsig);
	sigaddset(&newsig, SIGINT);
	sigaddset(&newsig, SIGTERM);
	//pthread_sigmask(SIG_BLOCK, &newsig, NULL);
	//set the no op interrupt handler
	struct sigaction act;
	act.sa_handler = dummyInterrupt;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	// unblock the signals, this is done so that accept will return when a SIGINT occur
	pthread_sigmask(SIG_UNBLOCK, &newsig, NULL);

	if(isTraceFile){
		//open the trace file and read the numbe rof customers
		tracefile = fopen (filename, "r");
		if(tracefile == NULL){
			printf("ERROR: file %s open fails\n", filename);
			return -1;
		}
		char line[80];
		memset(line, 0, 80);
		fgets(line, 80, tracefile);//in case of error exit
		TrimBlanks(line);
		sscanf (line, "%d", &nCustomer);//in case of error exit
	}
	//if numbe rof customer is 0  return
	if(nCustomer <=0){
		printf("ERROR: No Customer\n");
		return -1;
	}
	// print start message
	printStartMsg();
	//initialize random numbe rgenerator
	InitRandom(seedval);
	//initialize the lock and conditons
	pthread_mutex_init(&printLock, NULL);
	pthread_mutex_init(&qLock, NULL);
	pthread_cond_init(&qFilled, NULL);
	pthread_mutex_init(&intLock, NULL);
	pthread_cond_init(&intCond, NULL);

	//initialize the srevers and interrupt handler thread
	int t = 1;
	pthread_create(&server1Thread, NULL, serverThreadHandler, (void*)t);
	if(!isSingleServer){
		t = 2;
		pthread_create(&server2Thread, NULL, serverThreadHandler, (void*)t);
	}

	pthread_create(&sigintThread, NULL, interruptThreadHandler, (void*)t);
	//set the start time
	memset(&startTime, 0, sizeof(struct timeval));
	gettimeofday(&startTime, &tz);
	//print start message
	printtimemsg(&startTime, "emulation begins\n");
	return 0;
}
//this is a cleanup routine
void destroy(){
	//close file and dstroy all mutex and condition variables
	if(isTraceFile){
		fclose(tracefile);
	}

	pthread_mutex_destroy(&printLock);
	pthread_mutex_destroy(&qLock);
	pthread_cond_destroy(&qFilled);
	pthread_mutex_destroy(&intLock);
	pthread_cond_destroy(&intCond);
}
//method for printing start message
void printStartMsg(){
	//acuire lock
	pthread_mutex_lock (&printLock);

	printf("Parameters:\n");
	if(!isTraceFile){
		printf("       lambda = %0.02f\n", lambda);
		printf("       mu = %0.02f\n", mu);
	}

	if(isSingleServer){
		printf("       system = M/M/1\n");
	}
	else{
		printf("       system = M/M/2\n");
	}

	if(!isTraceFile){
		printf("       seed = %ld\n", seedval);
	}
	printf("       size = %d\n", qsize);
	printf("       number = %d\n", nCustomer);
	if(!isTraceFile){
		if(distr == DISTR_EXP){
			printf("       distribution = exp\n");
		}
		else{
			printf("       distribution = det\n");
		}
	}
	//release lock
	pthread_mutex_unlock (&printLock);
}
//method for printing a string with time elapsed from start of the emulation
void printtimemsg(struct timeval* tv, char* msg){
	//acuire lock
	pthread_mutex_lock (&printLock);
	struct timeval temptv;
	timeElapsed(&startTime, tv, &temptv);
	printf("%08d.%03dms: %s", (int)(temptv.tv_sec*1000 + temptv.tv_usec/1000), (int)temptv.tv_usec%1000, msg);
	//release lock
	pthread_mutex_unlock (&printLock);
}

/*parsing the command line
mm2 [-lambda lambda] [-mu mu] [-s] \
	[-seed seedval] [-size sz] \
	[-n num] [-d {exp|det}] [-t tsfile]
*/
int ProcessOptions(int argc, char **argv){
	//escaping the executable name
	argc--; argv++;

	//the number of arguments consumed in one iteration
	int arguments = 1;
	for(;argc > 0; argc-=arguments, argv += arguments){
		if (!strcmp(*argv, "-lambda")) {
			if(argc == 1){
				fprintf(stderr, "Error: Lambda value missing.\n");
        	    return (-1);
			}
			if(isalpha(*(*(argv + 1)))){
				fprintf(stderr, "Error: bad Lambda value.\n");
				return (-1);
			}

			lambda = strtod(*(argv + 1), NULL);
			arguments = 2;
		}
		else if (!strcmp(*argv, "-mu")) {
			if(argc == 1){
				fprintf(stderr, "Error: Mu value missing.\n");
				return (-1);
			}
			if(isalpha(*(*(argv + 1)))){
				fprintf(stderr, "Error: bad Mu value.\n");
				return (-1);
			}
			mu = strtod(*(argv + 1), NULL);
			arguments = 2;
		}
		else if (!strcmp(*argv, "-s")) {
			isSingleServer = 1;
			arguments = 1;
		}
		else if (!strcmp(*argv, "-seed")) {
			if(argc == 1){
				fprintf(stderr, "Error: Seed value missing.\n");
				return (-1);
			}
			if(isalpha(*(*(argv + 1)))){
				fprintf(stderr, "Error: bad Seed value.\n");
				return (-1);
			}
			seedval = atol(*(argv + 1));
			if(seedval < 0){
				fprintf(stderr, "Error: Seed value can't be negative.\n");
				return (-1);
			}
			arguments = 2;
		}
		else if (!strcmp(*argv, "-size")) {
			if(argc == 1){
				fprintf(stderr, "Error: Size value missing.\n");
				return (-1);
			}
			if(isalpha(*(*(argv + 1)))){
				fprintf(stderr, "Error: bad Size value.\n");
				return (-1);
			}
			qsize = atol(*(argv + 1));
			arguments = 2;
		}
		else if (!strcmp(*argv, "-n")) {
			if(argc == 1){
				fprintf(stderr, "Error: N value missing.\n");
				return (-1);
			}
			if(isalpha(*(*(argv + 1)))){
				fprintf(stderr, "Error: bad N value.\n");
				return (-1);
			}
			nCustomer = atol(*(argv + 1));
			arguments = 2;
		}
		else if (!strcmp(*argv, "-d")) {
			if(argc == 1){
				fprintf(stderr, "Error: Distribution specification missing.\n");
				return (-1);
			}
			if(!strcmp(*(argv + 1), "exp")){
				distr = DISTR_EXP;
			}
			else if(!strcmp(*(argv + 1) , "det")){
				distr = DISTR_DET;
			}
			else{
				 fprintf(stderr, "Error:Invalid distribution: %s\n", *(argv + 1));
				return (-1);
			}
			arguments = 2;
		}
		else if (!strcmp(*argv, "-t")) {
			isTraceFile = 1;
			if(argc == 1){
				fprintf(stderr, "Error: Trace file missing.\n");
				return (-1);
			}
			strncpy(filename, *(argv + 1), 512);
			filename[512] = '\n';
			arguments = 2;
		}
		else{
			fprintf(stderr, "Error: Invalid commandline option: %s\n", *(argv + 1));
			return (-1);
		}
	}
	return 0;
}
