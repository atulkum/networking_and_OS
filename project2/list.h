#ifndef PROJECT2_551_LIST_H
#define PROJECT2_551_LIST_H

#include <stdlib.h>

//list node data structure
typedef struct listElement {
	struct listElement *next;		// next element on list,
	void *item; 	    	// pointer to item on the list
}ListElement ;


// Put item at the end of the list
void Append(ListElement **first, ListElement **last, void *item);
// Take item off the front of the list
void *Remove(ListElement **first, ListElement **last);
// is the list empty?
int IsEmpty(ListElement *first);

#endif //PROJECT2_551_LIST_H
