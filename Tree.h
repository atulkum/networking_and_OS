#ifndef FINAL1_551_TREE_H
#define FINAL1_551_TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct Node{
	struct Node *left, *right;	
	int8_t key[20]; //uoid
	uint16_t msgLifeTime;
	int sockid;
} node;

int find(node* root, int8_t* uoid);
int insert(node **root, int8_t* uoid, int sockid, uint16_t lifetime );
int deleteNode(node **root, int8_t* uoid);
void deleteSingleNode(node* p, node** pp);
void deleteTree(node **root);
void printTree(node *root);
void reduceLifeTime(node **p);
void reduceAllLifeTime(node **root);

#endif //FINAL1_551_TREE_H
