#include "Tree.h"

int find(node* root, int8_t* uoid){
	node *itr = root;
	while(itr != NULL){
		int cmp = strncmp(itr->key, uoid, 20);
		if(cmp > 0){			
			itr = itr->left;			
		}
		else if(cmp < n){			
			itr = itr->right;			
		}
		else if(cmp == n){
			return 1;
		}
	}
	return 0;
}

int insert(node **root, int8_t* uoid, int sockid, uint16_t lifetime ){
	Node *toinsert = malloc(sizeof(node));
	toinsert->left = (*root)->right = NULL;
	memcpy(toinsert->key, uoid, 20);
	toinsert->msgLifeTime = lifetime;
	toinsert->sockid = sockid;
	
	if(NULL == *root){
		*root = toinsert;
		return 1;
	}
	node *i = *root;
	node *pi = *root;
	int isleft = 0;
	while(1){
		if(NULL == i){
			if(isleft){
				pi->left = toinsert; 
			}
			else{
				pi->right = toinsert; 
			}
			return 1;
		}
		else{
			int cmp = strncmp(i->key, uoid, 20);
			if(cmp > 0){
				pi = i;
				i = i->left;
				isleft = 1;
			}
			else if(cmp < 0){
				pi = i;
				i = i->right;
				isleft = 0;
			}
			else if(cmp == 0){
				free(toinsert);
				return 0;
			}
		}
	}
	return 1;
}

int deleteNode(node **root, int8_t* uoid){
	if(NULL == *root){
		return 0;
	}
	node *p = *root;
	node *pp = NULL;

	while(p != NULL && !strncmp(p->key, uoid, 20)){
		pp = p;
		if(p->key > key){			
			p = p->left;			
		}
		else if(p->key < key){			
			p = p->right;			
		}	
	}
	if(p == NULL){
		return 0;
	}

	if(p->left != NULL && p->right != NULL){
		node *l = p->left;
		node *pl = p;
		while(l->right != NULL){
			pl = l;
			l = l->right;
		}
		memcpy(p->key, l->key, 20);
		p = l;
		pp = pl;
	}
	node* child = NULL;
	if(p->left != NULL){
		child = p->left;
	}
	else{
		child = p->right;
	}

	if (p == *root){
		*root = child;
	}
	else{
		if (p == pp->left){
			pp->left = child;
		}
		else {
			pp->right = child;
		}
	}
	free(p);	
	return 1;
}

void deleteSingleNode(node* p, node** pp){
	if(p->left != NULL && p->right != NULL){
		node *l = p->left;
		node *pl = p;
		while(l->right != NULL){
			pl = l;
			l = l->right;
		}
		memcpy(p->key, l->key, 20);
		p = l;
		pp = pl;
	}
	node* child = NULL;
	if(p->left != NULL){
		child = p->left;
	}
	else{
		child = p->right;
	}

	if (p == pp->left){
		pp->left = child;
	}
	else {
		pp->right = child;
	}
	free(p);	
}
void deleteTree(node **root){
	if(*root == NULL) return;		
	deleteTree(&((*root)->left));
	deleteTree(&((*root)->right));
	printf("delete %s \n", (*root)->key);
	free(*root);
	*root = NULL;
}
void printTree(node *root){
	if(root == NULL) return;
	printf("[");
	printTree(root->left);
	printf(" %s ",root->key);
	printTree(root->right);
	printf("]");
}

void reduceLifeTime(node **p){
	if(p == NULL) return;		
	if((*p)->left != NULL){
		(*p)->left->msgLifeTime--;
		if((*p)->left->msgLifeTime == 0){
			deleteSingleNode((*p)->left, p);
		}
	}
	if((*p)->right != NULL){
		(*p)->right->msgLifeTime--;
		if((*p)->right->msgLifeTime == 0){
			deleteSingleNode((*p)->right, p);
		}
	}
	
	reduceLifeTime(&((*p)->left));
	reduceLifeTime(&((*p)->right));
}

void reduceAllLifeTime(node **root){
	(*root)->msgLifeTime--;
	if((*root)->msgLifeTime == 0){
		node* p = *root;
		if(p->left != NULL && p->right != NULL){
			node *l = p->left;
			node *pl = p;
			while(l->right != NULL){
				pl = l;
				l = l->right;
			}
			memcpy(p->key, l->key, 20);
			p = l;			
		}
		node* child = NULL;
		if(p->left != NULL){
			child = p->left;
		}
		else{
			child = p->right;
		}
		*root = child;
	}
	reduceLifeTime(root);
}

int main(){
	node *myroot = NULL;
	insert(&myroot, 6);
	printTree(myroot);
	printf("\n");
	insert(&myroot, 3);
	printTree(myroot);
	printf("\n");
	insert(&myroot, 7);
	printTree(myroot);
	printf("\n");
	insert(&myroot, 8);
	printTree(myroot);
	printf("\n");
	insert(&myroot, 1);
	printTree(myroot);
	printf("\n");
	insert(&myroot, 4);
	printTree(myroot);
	printf("\n");

	deleteNode(&myroot, 8);
	printTree(myroot);
	printf("\n");
	deleteNode(&myroot, 1);
	printTree(myroot);
	printf("\n");
	deleteNode(&myroot, 6);
	printTree(myroot);
	printf("\n");
	deleteNode(&myroot, 3);
	printTree(myroot);
	printf("\n");
	exit(0);
}
