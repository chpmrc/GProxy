/**
 * Useful functions to manipulate lists of packets
 */
 
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
 
#define PAYLOAD_SIZE 65000 /* Careful! There might be an error as "Message too long" */
 
typedef struct Pkt {
	unsigned int id;
	unsigned char type;
	char body[PAYLOAD_SIZE];
} Pkt;

typedef struct Node {
	Pkt *packet;
	struct Node *next, *prev, *head;
	int length; /* Only used in heads */
} Node;

 
Node *allocPkt(int id, char type, char *body){
	Node *toAlloc = (Node *)malloc(sizeof(Node));
	Pkt *packet = (Pkt *)malloc(sizeof(Pkt));
	if (toAlloc == NULL){
		perror("Error with malloc()");
	}
	toAlloc->next = toAlloc;
	toAlloc->prev = toAlloc;
	toAlloc->length = -1;
	packet->id = id;
	packet->type = type;
	if (body != NULL){
		strcpy(packet->body, body);
	}
	toAlloc->packet = packet;
	return toAlloc;
}

Node *allocHead(){
	Node *toAlloc = (Node *)malloc(sizeof(Node));
	if (toAlloc == NULL){
		perror("Error with malloc()");
	}
	toAlloc->next = toAlloc;
	toAlloc->prev = toAlloc;
	toAlloc->length = 0;
	return toAlloc;
}

Pkt *getPkt(Node *node){
	return node->packet;
}
 
void appendPkt(Node *head, Node *new){
	new->prev = head->prev;
	new->next = head;
	head->prev->next = new;
	head->prev = new;
	new->head = head;
	if (head->length >= 0){
		head->length += 1;
	}
}

Node *removePkt(Node *toRemove){
	Node *head = toRemove->head;
	if (toRemove == NULL) return NULL;
	if (toRemove->prev != NULL){
		toRemove->prev->next = toRemove->next;
	}
	if (toRemove->next != NULL){
		toRemove->next->prev = toRemove->prev;
	}
	toRemove->prev = NULL;
	toRemove->next = NULL;
	toRemove->head = NULL;
	head->length -= 1;
	return toRemove;
}

void clearPkt(Node *toClear){
	free(toClear->packet);
	free(toClear);
}

Node *searchPktById(Node *head, int id){
	/* Start from the first real node */
	Node *iter = head->next;
	while (iter != head && iter->packet->id != id){
		iter = iter->next;
	}
	if (iter == head) return NULL;
	return iter;
}

void insertPktById(Node *head, Node *new){
	/* Start from the first real node */
	Node *iter = head->next;
	while (iter != head && iter->packet->id < new->packet->id){
		iter = iter->next;
	}
	iter->prev->next = new;
	new->prev = iter->prev;
	iter->prev = new;
	new->next = iter;
	new->head = head;
	head->length += 1;
}

Node *removePktById(Node *head, int id){
	Node *temp = searchPktById(head, id);
	removePkt(temp);
	return temp;
}

void printNode(Node *n){
	printf("TEST: %s\n", n->packet->body);
}

/* Foreach implementation with callback */
void forEach(Node *head, void (*callbackPtr)(Node *)){
	Node *iter = head->next;
	while (iter != head){
		(*callbackPtr)(iter);
		iter = iter->next;
	}
}

void printList(Node *head){
	Node *iter = head->next;
	printf("============= LIST OF PACKETS (length: %d) =============\n", head->length);
	while (iter != head){
		printf("[\n\tid: %d, \tbody: %s\n]\n", iter->packet->id, iter->packet->body);
		iter = iter->next;
	}
	printf("============= END OF LIST =============\n\n");
}
