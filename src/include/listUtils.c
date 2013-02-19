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
	struct Node *next, *prev;
} Node;

 
Node *allocPkt(int id, char type, char *body){
	Node *toAlloc = (Node *)malloc(sizeof(Node));
	Pkt *packet = (Pkt *)malloc(sizeof(Pkt));
	if (toAlloc == NULL){
		perror("Error with malloc()");
	}
	toAlloc->next = toAlloc;
	toAlloc->prev = toAlloc;
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
}

Node *removePkt(Node *toRemove){
	if (toRemove->prev != NULL){
		toRemove->prev->next = toRemove->next;
	}
	if (toRemove->next != NULL){
		toRemove->next->prev = toRemove->prev;
	}
	toRemove->prev = NULL;
	toRemove->next = NULL;
	return toRemove;
}

void clearPkt(Node *toClear){
	removePkt(toClear);
	free(toClear->packet);
	free(toClear);
}

Node *searchPktById(Node *head, int id){
	/* Don't start from the dummy node */
	Node *iter = head->next;
	while (iter != head && iter->packet->id != id){
		iter = iter->next;
	}
	if (iter == head) return NULL;
	return iter;
}

void printList(Node *head){
	Node *iter = head->next;
	printf("============= LIST OF PACKETS =============\n");
	while (iter != head){
		printf("[\n\tid: %d, \tbody: %s\n]\n", iter->packet->id, iter->packet->body);
		iter = iter->next;
	}
	printf("============= END OF LIST =============\n\n");
}
