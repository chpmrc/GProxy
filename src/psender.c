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

#define PAYLOAD_SIZE 65510 /* Computed as IP_total_size - IP_max_header_size - id_size - type_size = 65535 - 20 - 4 - 1 */

/* MAIN STUFF */
struct sockaddr_in to, from, source, sourceClient; /* Indirizzo del socket locale e remoto del ritardatore e del sender */ 
int ritardatore, sender, connectedSender; /* socket descriptor verso/da il ritardatore e dal sender */
char sourceAddress[] = "0.0.0.0"; unsigned short int sourcePort = 59000;
char fromAddress[] = "0.0.0.0"; unsigned short int fromPort = 60000;
char toAddress[] = "127.0.0.1"; unsigned short int toPort = 63000;

/* DATA STRUCTURES */
typedef struct Pkt {
	unsigned int id;
	unsigned char type;
	char body[PAYLOAD_SIZE];
} Pkt;


/* UTILS */
extern char *logFilePath;
char recvBuffer[PAYLOAD_SIZE], sendBuffer[PAYLOAD_SIZE];
int trueOpt = 1;
int sharedError; /* Variabile di riconoscimento errori condivisa, serve per quelle funzioni che non ritornano interi */
int readCounter, sentCounter;
socklen_t toLen = sizeof(to);
socklen_t sourceClientLen = sizeof(sourceClient);

/* SELECT RELATED */
fd_set canRead, canReadCopy;
struct timeval timeout = { 2, 0 };
int selectResult;
int maxFd;

int main(){
	/* INIT */
	
	/* Cambia il path del file di log */
	logFilePath = "./psenderLog.txt";
	
	/* Creo il socket descriptor per il ritardatore e ne setto le specifiche opzioni */
	if ((ritardatore = getSocket(SOCK_DGRAM)) == SOCKERROR){
		printError("creazione fallita per il socket da/verso il ritardatore! Errore");
	}
	printLog("socket da/verso il ritardatore correttamente creato!");
	
	/* Creo il socket descriptor per la sorgente/sender */
	if ((sender = getSocket(SOCK_STREAM)) == SOCKERROR){
		printError("creazione fallita per il socket dal sender! Errore");
	}
	printLog("socket dal sender correttamente creato!");
	
		
	/* Inizializzo la struttura per il socket in ricezione dal sender/sorgente */
	source = getSocketAddress(sourceAddress, sourcePort);
	if (sharedError){
		printError("creazione dell'indirizzo associato al socket dal sender fallita! Errore");
	}
	printLog("indirizzo associato al socket dal sender correttamente creato!");
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
	printLog("Sender connected! Waiting for data...");
	printf("Client port: %d\n", ntohs(sourceClient.sin_port));
	
	/* Add the sockets into the sets of descriptors */
	FD_ZERO(&canRead);
	FD_SET(connectedSender, &canRead); /* Warning! We must wait data on the connected socket, not the listening one! */
	FD_SET(ritardatore, &canRead);
	maxFd = (connectedSender < ritardatore)? ritardatore : connectedSender;
	
	/* DEBUG AND TEMPORARY STUFF */
	
	/* Leggo i datagram */
	while(TRUE){
		/* Make a copy of the fd_set to avoid modifying the original one */
		canReadCopy = canRead;
		memset(recvBuffer, 0, readCounter);
		
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
				/* Here we have to build N packets and put them into a list, then continue the execution */
				readCounter = recv(connectedSender, recvBuffer, 65000, MSG_DONTWAIT);
				if (readCounter == 0){
					close(sender);
				}
				sentCounter = sendto(ritardatore, recvBuffer, readCounter, 0, (struct sockaddr *)&to, sizeof(to));
				if (sentCounter < 0){
					printError("Error while sending data to the ritardatore!");
				}
				if (sentCounter == 0){
					close(ritardatore); /* The socket of the ritardatore must be closed only when all the packets are sent (from the list) */
					printLog("ALL DONE!");
					exit(0);	
				}
			}
			
			/* Check if we can receive data from the ritardatore */
			if (FD_ISSET(ritardatore, &canReadCopy)){
				readCounter = recv(ritardatore, recvBuffer, 65000, MSG_DONTWAIT);
				/* Here we have to verify if an ICMP has been received and
				 * search the id into the "sent" list. Once we find the guilty
				 * packet we just put it again into the "toSend" list and switch port.
				 * If an ACK is received the packet is removed from the "sent" list. */
			}			
		}
	}
}
