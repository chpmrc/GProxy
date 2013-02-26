#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include "include/globalUtils.c"
#include "include/listUtils.c"

/* MAIN STUFF */
struct sockaddr_in to, from, dest; /* Indirizzo del socket locale e remoto del ritardatore e del receiver */ 
int ritardatore, receiver; /* socket descriptor verso/da il ritardatore e verso receiver */
char destAddress[] = "127.0.0.1"; unsigned short int destPort = 64000;
char fromAddress[] = "0.0.0.0"; unsigned short int fromPort = 63000;
char toAddress[] = "127.0.0.1"; unsigned short int toPort = 60000;

/* UTILS */
extern char *logFilePath;
char recvBuffer[PAYLOAD_SIZE], sendBuffer[PAYLOAD_SIZE];
int trueOpt = 1, connectStatus;
int sharedError; /* Variabile di riconoscimento errori condivisa, serve per quelle funzioni che non ritornano interi */
int recvCounter, sendCounter, lastId; /* WARNING: don't use size_t since it's an unsigned numeric type! Not suitable for possible error return values (e.g. -1) */
socklen_t toLen = sizeof(to);
socklen_t destLen = sizeof(dest);

Node *toAck, *ackD; /* List heads */
Node *iter, *current; /* Simple iterators */

Pkt ack; /* Used to send ack to the psender */

/* SELECT RELATED */
fd_set canRead, canWrite, canExcept, canReadCopy, canWriteCopy, canExceptCopy;
struct timeval timeout = { 2, 0 };
int selectResult;
int maxFd;

/**
 * Simply send an ack for the given packet and remove it from
 * the list
 */
void ackPkt(Node *current){
	/* Build and send the ack */
	sprintf(ack.body, "%d%c%d", 0, 'B', current->packet->id);
	/* Since we have to send on a different port we can't use the default binding, sendto is mandatory */
	sendto(ritardatore, &ack, sizeof(Pkt), 0, (struct sockaddr *)&to, toLen);
	removePkt(head);
	clearPkt(head);
}

int main(){
	
	/* Initialize local structures */
	toAck = allocHead();
	
	/* Cambia il path del file di log */
	logFilePath = "./preceiverLog.txt";
	
	/* Creo il socket descriptor per il ritardatore e ne setto le specifiche opzioni */
	if ((ritardatore = getSocket(SOCK_DGRAM)) == SOCKERROR){
		printError("creazione fallita per il socket da/verso il ritardatore! Errore");
	}
	printLog("socket da/verso il ritardatore correttamente creato!");
	
	/* Creo il socket descriptor per la sorgente/receiver */
	if ((receiver = getSocket(SOCK_STREAM)) == SOCKERROR){
		printError("creazione fallita per il socket dal receiver! Errore");
	}
	printLog("socket dal receiver correttamente creato!");
	
		
	/* Initialize the address to be used to connect to the receiver */
	dest = getSocketAddress(destAddress, destPort);
	if (sharedError){
		printError("creazione dell'indirizzo associato al socket dal receiver fallita! Errore");
	}
	printLog("indirizzo associato al socket dal receiver correttamente creato!");
	/* Inizializzo la struttura per il socket in ricezione dal ritardatore */
	from = getSocketAddress(fromAddress, fromPort);
	if (sharedError){
		printError("creazione dell'indirizzo associato al socket dal ritardatore fallita! Errore");
	}
	printLog("indirizzo associato al socket dal ritardatore correttamente creato!");
	/* Inizializzo la struttura per il socket in invio verso il ritardatore */
	to = getSocketAddress(toAddress, toPort);
	if (sharedError){
		printError("creazione dell'indirizzo associato al socket verso il ritardatore fallita! Errore");
	}
	printLog("indirizzo associato al socket verso il ritardatore correttamente creato!");
	
	/* BINDS AND CONNECTS */
	
	/* Lego il socket dal ritardatore ad un indirizzo in ricezione */
	bind(ritardatore, (struct sockaddr *)&from, sizeof(from));
	
	/* First of all establish a connection with the dest (only one connection at a time is allowed) */
	connectStatus = connect(receiver, (struct sockaddr *)&dest, destLen);
	if (connectStatus < 0){
		printError("There was an error with the connect(). See details below:");
	}
	printLog("Connected to Receiver! Waiting for data...");
	
	/* Add the sockets into the sets of descriptors */
	FD_ZERO(&canRead); FD_ZERO(&canWrite);
	FD_SET(receiver, &canWrite);
	FD_SET(ritardatore, &canRead);
	maxFd = (receiver < ritardatore)? ritardatore : receiver;
	
	/* Leggo i datagram */
	while(TRUE){
		/* Make a copy of the fd_set to avoid modifying the original one (struct copy) */
		canReadCopy = canRead;
		canWriteCopy = canWrite;

		/* Main (and only) point of blocking from now on */
		selectResult = select(maxFd+1, &canReadCopy, &canWriteCopy, NULL, NULL);
		
		/* Check for errors */
		if (selectResult < 0){
			printError("There was an error with the select function!");
		}
		
		/* Check for active sockets */
		if (selectResult > 0){
				
			/* Check if we can read data from the ritardatore */
			if (FD_ISSET(ritardatore, &canReadCopy)){
				current = allocPkt(0, 'B', NULL);
				recvCounter = recv(ritardatore, current->packet, sizeof(Pkt), 0);
				printf("Received id %d\n", current->packet->id);
				if (recvCounter > 0){
					insertPktById(toAck, current);
					lastId = current->packet->id;
					printList(toAck);
				}
			}
			
			/* Check if we can send data to the receiver */
			if (FD_ISSET(receiver, &canWriteCopy)){
				/* Scan the packets list */	
				
				
				
				
				
				if (lastId != -1){
					current = searchPktById(toAck, lastId);
					lastId = -1;
					strcpy(recvBuffer, current->packet->body);
					send(receiver, recvBuffer, PAYLOAD_SIZE, 0);
					ackPkt(current);
					memset(recvBuffer, 0, 100);
				}
			}
		}
	}
}
