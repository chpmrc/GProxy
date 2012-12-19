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
struct sockaddr_in to, from, dest, destClient; /* Indirizzo del socket locale e remoto del ritardatore e del receiver */ 
int ritardatore, receiver, connectedReceiver; /* socket descriptor verso/da il ritardatore e verso receiver */
char destAddress[] = "0.0.0.0"; unsigned short int destPort = 64000;
char fromAddress[] = "0.0.0.0"; unsigned short int fromPort = 63000;
char toAddress[] = "127.0.0.1"; unsigned short int toPort = 62000;

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
int readCounter, sentCounter; /* WARNING: don't use size_t since it's an unsigned numeric type! Not suitable for possible error return values (e.g. -1) */
socklen_t toLen = sizeof(to);
socklen_t destClientLen = sizeof(destClient);

/* SELECT RELATED */
fd_set canRead, canWrite, canExcept, canReadCopy, canWriteCopy, canExceptCopy;
struct timeval timeout = { 2, 0 };
int selectResult;
int maxFd;

int main(){
	/* INIT */
	
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
	
		
	/* Inizializzo la struttura per il socket in ricezione dal receiver/sorgente */
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
	
	/* BINDS AND LISTENS */
	
	/* Lego il socket dal ritardatore ad un indirizzo in ricezione */
	bind(ritardatore, (struct sockaddr *)&from, sizeof(from));
	
	/* First of all establish a connection with the dest (only one connection at a time is allowed) */
	connectedReceiver = connect(receiver, (struct sockaddr *)&destClient, destClientLen);
	printf("%d\n", connectedReceiver);
	if (connectedReceiver < 0){
		printError("There was an error with the accept(). See details below:");
	}
	printLog("Connected to Receiver! Waiting for data...");
	
	/* Add the sockets into the sets of descriptors */
	FD_ZERO(&canRead); FD_ZERO(&canWrite); FD_ZERO(&canExcept);
	FD_SET(receiver, &canWrite);
	FD_SET(receiver, &canExcept);
	FD_SET(ritardatore, &canRead);
	FD_SET(ritardatore, &canExcept);
	maxFd = (receiver < ritardatore)? ritardatore : receiver;
	
	FILE *out = fopen("./sent.it", "w");
	
	/* Leggo i datagram */
	while(TRUE){
		/* Make a copy of the fd_set to avoid modifying the original one */
		canReadCopy = canRead;
		canWriteCopy = canWrite;
		canExceptCopy = canExcept;
		/* Main (and only) point of blocking from now on */
		selectResult = select(maxFd+1, &canReadCopy, &canWriteCopy, &canExceptCopy, NULL); /* TODO check the result */
		
		readCounter = recv(ritardatore, recvBuffer, 65000, MSG_DONTWAIT);
		
		if (readCounter > 0){			
			fwrite(recvBuffer, readCounter, 1, out);
			fflush(out);
			memset(recvBuffer, 0, readCounter);
		}
	}
}
