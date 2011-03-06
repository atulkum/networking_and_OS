#include "list.h"
//Idea of the code is taken form the Nachos OS list impelmentaion
//from CSCI402 class

//this method will add a node at the head of the linked list
void Append(ListElement **first,ListElement **last, void *item){

    ListElement *element = (ListElement*)malloc(sizeof(ListElement));

	element->item = item;
	element->next = NULL;

    if (IsEmpty(*first)) {		// list is empty
		*first = element;
		*last = element;
    } else {			// else put it after last
		(*last)->next = element;
		*last = element;
    }
}
//this will check is the linked list empty by checking fisrt pointer
int IsEmpty(ListElement *first){
    if(first == NULL){
		return 1;
	}
	else{
		return 0;
	}
}
//This will remove the first element from the queue and return the item
void *Remove(ListElement **first, ListElement **last){
	if (IsEmpty(*first)){
		return NULL;
	}

	ListElement *element = *first;
	void *thing = (*first)->item;

	if (*first == *last) {
		*first = NULL;
		*last = NULL;
	} else {
		*first = element->next;
	}
	free(element);

	return thing;
}

