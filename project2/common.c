#include "common.h"
//method for initializing the randomize function using the seed passed
void InitRandom(long l_seed)
{
	if (l_seed == 0L) {
		time_t localtime=(time_t)0;

		time(&localtime);
		srand48((long)localtime);
	} else {
		srand48(l_seed);
	}
}
//this method will return time interval used for getting
//arrival time  and service time
int GetInterval(int exponential, double rate){
	if(rate == 0){
		return 10000;
	}
	if (exponential) {
		double dval=(double)drand48();
		//call exponential distribution method
		return ExponentialInterval(dval, rate);
	} else {
		//if the distribution is deterministic
		double millisecond=((double)1000)/rate;
		//adjust the time in [1, 10000]
		if(millisecond < 1) millisecond = 1;
		else if(millisecond > 10000) millisecond = 10000;
		//round it and return
		return round(millisecond);
	}
}
//this method calculate the time in exponential distribution case
int ExponentialInterval(double dval, double rate){
	//using the formula given in the spec it calculate the time in miliseconds given the rate
	double millisecond = - ((log(1 - dval)*1000) / rate);
	//adjust the time in [1, 10000]
	if(millisecond < 1) millisecond = 1;
	else if(millisecond > 10000) millisecond = 10000;
	//round it and return
	return round551(millisecond);
}
// this method will remove the leading and trailing whitespace from the string
int TrimBlanks(char *str){
    size_t len = 0;
    char *front = str;
    char *end = NULL;
	//if string is null or empty return
    if( str != NULL ||  str[0] != '\0'){
		len = strlen(str);
		end = str + len - 1 ;
		//remove starting whitespace
		while( isspace(*front)){
			front++;
		}
		//remove trailing whitespace
		while( isspace(*end) ){
			end--;
			if(end == front){
				break;
			}
		}
		//put '\0' at the end of the string
		if( str + len - 1 != end ){
			*(end + 1) = '\0';
		}
		else if( front != str &&  end == front ){
			*str = '\0';
		}
		end = str;
		//copy the string at the satrt location, so that string will start from the start location
		if( front != str ){
			while( *front ) {
				*end++ = *front++;
			}
			*end = '\0';
		}
		return 0;
	}
	return -1;
}
//This will read the inter arrival time and the service time from the trace file
void readArrSer(char *str, int *a, int *s){
    char *front = str;
	char *newfront = str;
    if( str != NULL ||  str[0] != '\0'){
		//read till thre is no space
		while(!isspace(*front)){
			front++;
		}
		*front = '\0';
		//get the inter arrival time
		*a =  atoi(newfront);
		//if inter arrivale time is not in [1, 10000] adjust it
		if(*a < 1) *a = 1;
		else if(*a > 10000) *a = 10000;

		front++;
		while(isspace(*front)){
			front++;
		}
		//read service time
		*s =  atoi(front);
		//if service time is not in [1, 10000] adjust it
		if(*s < 1) *s = 1;
		else if(*s > 10000) *s = 10000;
	}
}
