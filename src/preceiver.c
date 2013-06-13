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
#include <time.h>
#include "include/globalUtils.c"
#include "include/listUtils.c"

/* MAIN STUFF */
struct sockaddr_in toRit, fromRit, dest; /* Indirizzo del socket locale e remoto del ritardatore e del receiver */ 
int ritardatore, receiver; /* socket descriptor verso/da il ritardatore e verso receiver */
char destAddress[] = "192.168.1.125"; unsigned short int destPort = 64000;
char fromRitAddress[] = "0.0.0.0"; unsigned short int fromRitPort = 63000;
char toRitAddress[] = "127.0.0.1"; unsigned short int toRitPort = 62000;

/* UTILS */
extern char *logFilePath;
char recvBuffer[PAYLOAD_SIZE], sendBuffer[PAYLOAD_SIZE];
int trueOpt = 1, connectStatus;
int sharedError; /* Variabile di riconoscimento errori condivisa, serve per quelle funzioni che non ritornano interi */
int recvCounter, sendCounter; /* WARNING: don't use size_t since it's an unsigned numeric type! Not suitable for possible error return values (e.g. -1) */
socklen_t toLen = sizeof(toRit);
socklen_t destLen = sizeof(dest);

Node *toAck; /* List heads */
Node *iter, *current; /* Simple iterators */
boolean result; /* Simple container for results */
int lastSentId = 0;
boolean finalize = FALSE;

time_t startTime, endTime;
boolean started = FALSE;



/* SELECT RELATED */
fd_set canRead, canWrite, canExcept, canReadCopy, canWriteCopy, canExceptCopy;
struct timeval timeout = { 2, 0 };
int selectResult;
int maxFd;

/**
 * Simply send an ack for the given packet and remove it fromRit
 * the list
 */
void ackPkt(Node *current){
	Pkt ack; /* Used to send ack to the psender */
	/* Build and send the ack */
	ack.id = 0;
	ack.type = 'B';
	sprintf(ack.body, "%d%c", current->packet->id, '\0');
	printf("Sending ack for ID %d\n", current->packet->id);
	/* Since we have to send on a different port we can't use the default binding, sendto is mandatory */
	sendto(ritardatore, &ack, sizeof(Pkt), 0, (struct sockaddr *)&toRit, toLen);
}

void sendToReceiver(Node *current){
	if (current != NULL){
		/* Don't send the ending packet! */
		printf("Receiving packet %d, lastSentId %d\n", current->packet->id, lastSentId);
		if (current->packet->id == lastSentId+1){
			sendCounter = send(receiver, current->packet->body, current->pktSize - HEADER_SIZE, 0); /* Only send the body! */
			switch(sendCounter){
				case 0:
					break;
					
				case -1:
					break;
					
				default:
					/* If I can send the packet toRit the receiver I also have to remove it */
					ackPkt(current);
					removeNode(current);
					clearNode(current);
					lastSentId++;
			}
		} else {
			/* In this case we just have to tell the psender not to send this packet again */
			ackPkt(current);
			if (current->packet->id <= lastSentId){
				removeNode(current);
				clearNode(current);
			}
		}
	}
}

int main(int argc, char *argv[]){
	
	/* Initialize local structures */
	toAck = allocHead();
	
	/* Cambia il path del file di log */
	logFilePath = "./preceiverLog.txt";
	
	/* Creo il socket descriptor per il ritardatore e ne setto le specifiche opzioni */
	if ((ritardatore = getSocket(SOCK_DGRAM)) == SOCKERROR){
		printError("creazione fallita per il socket da/verso il ritardatore! Errore");
	}
	printf("[LOG] socket da/verso il ritardatore correttamente creato!\n");
	
	/* Creo il socket descriptor per la sorgente/receiver */
	if ((receiver = getSocket(SOCK_STREAM)) == SOCKERROR){
		printError("creazione fallita per il socket dal receiver! Errore");
	}
	printf("[LOG] socket dal receiver correttamente creato!\n");
	
		
	/* Initialize the address toRit be used toRit connect toRit the receiver */
	dest = getSocketAddress(destAddress, destPort);
	if (sharedError){
		printError("creazione dell'indirizzo associato al socket dal receiver fallita! Errore");
	}
	printf("[LOG] indirizzo associato al socket del receiver correttamente creato: %s:%d\n", destAddress, destPort);
	/* Inizializzo la struttura per il socket in ricezione dal ritardatore */
	fromRit = getSocketAddress(fromRitAddress, fromRitPort);
	if (sharedError){
		printError("creazione dell'indirizzo associato al socket dal ritardatore fallita! Errore");
	}
	printf("[LOG] indirizzo associato al socket dal ritardatore correttamente creato: %s:%d\n", fromRitAddress, fromRitPort);
	/* Inizializzo la struttura per il socket in invio verso il ritardatore */
	toRit = getSocketAddress(toRitAddress, toRitPort);
	if (sharedError){
		printError("creazione dell'indirizzo associato al socket verso il ritardatore fallita! Errore");
	}
	printf("[LOG] indirizzo associato al socket verso il ritardatore correttamente creato: %s:%d\n", toRitAddress, toRitPort);
	
	/* BINDS AND CONNECTS */
	
	/* Lego il socket dal ritardatore ad un indirizzo in ricezione */
	bind(ritardatore, (struct sockaddr *)&fromRit, sizeof(fromRit));
	
	/* First of all establish a connection with the dest (only one connection at a time is allowed) */
	connectStatus = connect(receiver, (struct sockaddr *)&dest, destLen);
	if (connectStatus < 0){
		printError("There was an error with the connect(). See details below:");
	}
	printf("[LOG] Connected toRit Receiver! Waiting for data...\n");
	
	/* Add the sockets into the sets of descriptors */
	FD_ZERO(&canRead); FD_ZERO(&canWrite);
	FD_SET(receiver, &canWrite);
	FD_SET(ritardatore, &canRead);
	maxFd = (receiver < ritardatore)? ritardatore : receiver;
	
	/* Leggo i datagram */
	while(TRUE){
		/* Make a copy of the fd_set toRit avoid modifying the original one (struct copy) */
		canReadCopy = canRead;
		canWriteCopy = canWrite;

		/* Main (and only) point of blocking fromRit now on */
		selectResult = select(maxFd+1, &canReadCopy, &canWriteCopy, NULL, NULL);
		
		/* Compute transmission time */
		if (!started){
			time(&startTime);
			started = TRUE;
		}
		
		/* Check for errors */
		if (selectResult < 0){
			printError("There was an error with the select function!");
		}
		
		/* Check for active sockets */
		if (selectResult > 0){
				
			/* Check if we can read data fromRit the ritardatore */
			if (FD_ISSET(ritardatore, &canReadCopy)){
				/* Create a dummy packet to store the real one */
				current = allocNode(0, 'B', NULL, 0);
				memset(current->packet, 0, sizeof(Pkt));
				recvCounter = recv(ritardatore, current->packet, sizeof(Pkt), 0);
				/* Distinguish between body packets and ICMP packets */
				if (current->packet->type == 'B'){
					printf("Received ID %d\n", current->packet->id);
					if (recvCounter > 0){
						current->pktSize = recvCounter;
						if (strcmp(current->packet->body, endingBody) == 0){
							printf("FINALIZING TRANSMISSION...\n");
							finalize = TRUE;
						} else {
							result = insertNodeById(toAck, current);
							if (!result){
								printf("Node %d already present, ignoring...\n", current->packet->id);
							}
						}
					}
				} else {
					/* TODO It's an ICMP packet from the Ritardatore, resend the ack */
				}
			}
			
			/* Check if we can send data toRit the receiver */
			if (FD_ISSET(receiver, &canWriteCopy)){
				/* Scan the packets list */	
				forEach(toAck, &sendToReceiver);
				if (finalize && toAck->length == 0){
					/* CLOSE */
					printf("TRANSMISSION COMPLETE!\n");
					close(ritardatore);
					close(receiver);
					/* Report time */
					time(&endTime);
					printf("Packets received (size %d): %d | Time elapsed: %f\n", PAYLOAD_SIZE+HEADER_SIZE, lastSentId, difftime(endTime, startTime));
					exit(0);
				}
			}
		}
	}
}
