/**
 * Useful functions to manipulate lists of packets
 */
 
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
 
#define PAYLOAD_SIZE 65000 /* Careful! There might be an error as "Message too long" */
#define TRUE 1
#define FALSE 0

typedef int boolean;
 
typedef struct Pkt {
	unsigned int id;
	unsigned char type;
	char body[PAYLOAD_SIZE];
} Pkt;

typedef struct Node {
	Pkt *packet;
	struct Node *next, *prev, *head;
	int length; /* Only used in heads */
	int pktSize;
} Node;

 
Node *allocNode(int id, char type, char *body, int size){
	Node *toAlloc = (Node *)malloc(sizeof(Node));
	Pkt *packet = (Pkt *)malloc(sizeof(Pkt));
	if (toAlloc == NULL){
		perror("Error with malloc()");
	}
	toAlloc->next = toAlloc;
	toAlloc->prev = toAlloc;
	toAlloc->length = -1;
	toAlloc->pktSize = 0;
	packet->id = id;
	packet->type = type;
	if (body != NULL){
		strncpy(packet->body, body, size);
		toAlloc->pktSize = size;
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
	toAlloc->pktSize = -1;
	return toAlloc;
}

Pkt *getPkt(Node *node){
	return node->packet;
}
 
void appendNode(Node *head, Node *new){
	new->prev = head->prev;
	new->next = head;
	head->prev->next = new;
	head->prev = new;
	new->head = head;
	if (head->length >= 0){
		head->length += 1;
	}
}

Node *removeNode(Node *toRemove){
	if (toRemove != NULL){
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
	} else {
		return NULL;
	}
}

void clearNode(Node *toClear){
	if (toClear != NULL){
		free(toClear->packet);
		free(toClear);
	}
}

Node *searchNodeById(Node *head, int id){
	/* Start from the first real node */
	Node *iter = head->next;
	while (iter != head && iter->packet->id != id){
		iter = iter->next;
	}
	if (iter == head) return NULL;
	return iter;
}

boolean insertNodeById(Node *head, Node *new){
	/* Start from the first real node */
	boolean present = FALSE; /* For efficiency */
	Node *iter = head->next;
	while (iter != head && iter->packet->id < new->packet->id){
		if (iter != head && iter->packet->id == new->packet->id){
			present = TRUE;
			break;
		}
		iter = iter->next;
	}
	if (!present){
		iter->prev->next = new;
		new->prev = iter->prev;
		iter->prev = new;
		new->next = iter;
		new->head = head;
		head->length += 1;
		return TRUE;
	}
	return FALSE;
}

Node *removeNodeById(Node *head, int id){
	Node *temp = searchNodeById(head, id);
	removeNode(temp);
	return temp;
}

void printNode(Node *n){
	printf("TEST: %s\n", n->packet->body);
}

/* Foreach implementation with callback */
void forEach(Node *head, void (*callbackPtr)(Node *)){
	Node *iter = head->next, *next;
	while (iter != head){
		next = iter->next;
		(*callbackPtr)(iter);
		iter = next; /* Avoid segmentation fault if the callback modifies the list */
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
