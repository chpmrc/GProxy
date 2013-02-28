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

#define MAX_PACKETS 10

/* MAIN STUFF */
struct sockaddr_in to, from, source, sourceClient; /* Indirizzo del socket locale e remoto del ritardatore e del sender */ 
int ritardatore, sender, connectedSender; /* socket descriptor verso/da il ritardatore e dal sender */
char sourceAddress[] = "0.0.0.0"; unsigned short int sourcePort = 59000;
char fromAddress[] = "0.0.0.0"; unsigned short int fromPort = 60000;
char toAddress[] = "127.0.0.1"; unsigned short int toPort = 63000;

/* DATA STRUCTURES AND LIST HEADS */
char recvBuffer[PAYLOAD_SIZE];
Node *toSend, *toAck, *current, *endingNode;
Pkt currentAck;

/* UTILS */
extern char *logFilePath;
int trueOpt = 1;
int sharedError; /* Variabile di riconoscimento errori condivisa, serve per quelle funzioni che non ritornano interi */
int readCounter, 
	sentCounter, 
	packetsRead, 
	currentId;
socklen_t toLen = sizeof(to);
socklen_t sourceClientLen = sizeof(sourceClient);

boolean finalize = FALSE;

/* SELECT RELATED */
fd_set canRead, canReadCopy;
struct timeval timeout = { 2, 0 };
int selectResult;
int maxFd;

void sendPacket(Node *node){
	Pkt *pkt = node->packet;
	sentCounter = sendto(ritardatore, pkt, node->pktSize, 0, (struct sockaddr *)&to, sizeof(to));
	if (sentCounter < 0){
		perror("There was an error with sendto()");
		exit(1);
	} else {
		printf("Sent ID %d\n", pkt->id);
		if (strcmp(pkt->body, endingBody) == 0){
			close(ritardatore);
			exit(0);
		}
	}
}

int main(){
	/* INIT */
	
	/* Initialize local structures */
	toSend = allocHead();
	toAck = allocHead();
	currentId = 1; /* 0 is reserved! */
	
	/* Cambia il path del file di log */
	logFilePath = "./psenderLog.txt";
	
	/* Creo il socket descriptor per il ritardatore e ne setto le specifiche opzioni */
	if ((ritardatore = getSocket(SOCK_DGRAM)) == SOCKERROR){
		printError("creazione fallita per il socket da/verso il ritardatore! Errore");
	}
	printf("[LOG] socket da/verso il ritardatore correttamente creato!\n");
	
	/* Creo il socket descriptor per la sorgente/sender */
	if ((sender = getSocket(SOCK_STREAM)) == SOCKERROR){
		printError("creazione fallita per il socket dal sender! Errore");
	}
	printf("[LOG] socket dal sender correttamente creato!\n");
	
		
	/* Inizializzo la struttura per il socket in ricezione dal sender/sorgente */
	source = getSocketAddress(sourceAddress, sourcePort);
	if (sharedError){
		printError("creazione dell'indirizzo associato al socket dal sender fallita! Errore");
	}
	printf("[LOG] indirizzo associato al socket dal sender correttamente creato: %s:%d\n", sourceAddress, sourcePort);
	/* Inizializzo la struttura per il socket in ricezione dal ritardatore */
	from = getSocketAddress(fromAddress, fromPort);
	if (sharedError){
		printError("creazione dell'indirizzo associato al socket dal ritardatore fallita! Errore");
	}
	printf("[LOG] indirizzo associato al socket dal ritardatore correttamente creato: %s:%d\n", fromAddress, fromPort);
	/* Inizializzo la struttura per il socket in invio verso il ritardatore */
	to = getSocketAddress(toAddress, toPort);
	if (sharedError){
		printError("creazione dell'indirizzo associato al socket verso il ritardatore fallita! Errore");
	}
	printf("[LOG] indirizzo associato al socket verso il ritardatore correttamente creato: %s:%d\n", toAddress, toPort);
	
	/* BINDS AND LISTENS */
	
	/* Lego il socket dal ritardatore ad un indirizzo in ricezione */
	bind(ritardatore, (struct sockaddr *)&from, sizeof(from));
	
	/* Lego il socket dal sender ad un indirizzo in ricezione */
	bind(sender, (struct sockaddr *)&source, sizeof(source));
	listen(sender, 1);
	
	/* First of all estabilish a connection with the source (only one connection at a time is allowed) */
	connectedSender = accept(sender, (struct sockaddr *)&sourceClient, &sourceClientLen);
	if (connectedSender < 0){
		printError("There was an error with the accept(). See details below:");
	}
	printf("[LOG] Sender connected! Waiting for data...");
	printf("[LOG] Client port: %d\n", ntohs(sourceClient.sin_port));
	
	/* Add the sockets into the sets of descriptors */
	FD_ZERO(&canRead);
	FD_SET(connectedSender, &canRead); /* Warning! We must wait data on the connected socket, not the listening one! */
	FD_SET(ritardatore, &canRead);
	maxFd = (connectedSender < ritardatore)? ritardatore : connectedSender;
	
	/* Leggo i datagram */
	while(TRUE){
		/* Make a copy of the fd_set to avoid modifying the original one */
		canReadCopy = canRead;
		
		/* Main (and only) point of blocking from now on */
		selectResult = select(maxFd+1, &canReadCopy, NULL, NULL, NULL);
		
		/* Check for errors */
		if (selectResult < 0){
			printError("There was an error with the select function!");
		}
		/* Check for active sockets */
		if (selectResult > 0){
		
			/* Check if we can receive data from the sender */
			if (FD_ISSET(connectedSender, &canReadCopy)){
				/* Here we have to build n packets and put them into a list */
				/* Fill the buffer */
				readCounter = recv(connectedSender, recvBuffer, PAYLOAD_SIZE, MSG_DONTWAIT);
				printf("Letti %d byte\n", readCounter);
				/* Check cases */
				switch(readCounter){
					case 0:
						finalize = TRUE;
						break;
						
					case -1:
						/* ERROR */
						break;
						
					default:
						/* Build a node and append to the toSend list */
						current = allocNode(currentId, 'B', recvBuffer, readCounter+HEADER_SIZE);
						appendNode(toAck, current);
						/* printList(toAck); */ /* DEBUG */
						currentId++;
						
				}				
				
				/* Here we have to scan the list and send its packets */
				forEach(toAck, &sendPacket);
							
			}
			
			/* Check if we can receive data from the ritardatore */
			if (FD_ISSET(ritardatore, &canReadCopy)){
				readCounter = recv(ritardatore, &currentAck, sizeof(Pkt), MSG_DONTWAIT);
				/* Check the type of the packet */
				if (currentAck.type == 'B'){
					/* If we receive a body packet it must be an ack from the preceiver */
					printf("Received ACK for ID: %s\n", currentAck.body);
					current = removeNodeById(toAck, atoi(currentAck.body));
					clearNode(current);
					/* If we reached the end of the transmission send the final packet */
					if (finalize && toAck->length == 0){
						/* We can send the ending packet only when there are no packets left to be sent */
						printf("Finalizing transmission...\n");
						endingNode = allocNode(0, 'B', endingBody, strlen(endingBody)+HEADER_SIZE+1);
						sendPacket(endingNode);
						clearNode(endingNode);
					}
				} else {
					/* Otherwise we received an ICMP packet from the ritardatore */
				}
			}			
		}
	}
}
