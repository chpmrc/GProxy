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
#include "include/globalUtils.h"

struct sockaddr_in to, from, source, sourceClient; /* Indirizzo del socket locale e remoto del ritardatore e del sender */ 
int ritardatore, sender, connectedSender; /* socket descriptor verso/da il ritardatore e dal sender */
char sourceAddress[] = "0.0.0.0"; unsigned short int sourcePort = 59000;
char fromAddress[] = "0.0.0.0"; /* INADDR_ANY */ unsigned short int fromPort = 60000;
char toAddress[] = "127.0.0.1"; unsigned short int toPort = 61000;

/* UTILS */
char recvBuffer[1000], sendBuffer[1000];
int trueOpt = 1;
int sharedError; /* Variabile di riconoscimento errori condivisa, serve per quelle funzioni che non ritornano interi */
int readCounter, sentCounter;
socklen_t toLen = sizeof(to);
socklen_t sourceClientLen = sizeof(sourceClient);

/* SELECT */
fd_set canRead, canWrite, canExcept;
struct timeval timeout;
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
	
	/* Leggo i datagram */
	while(TRUE){
		/* Add the sockets into the sets of descriptors */
		FD_ZERO(&canRead); FD_ZERO(&canWrite); FD_ZERO(&canExcept);
		FD_SET(sender, &canRead); 
		FD_SET(sender, &canExcept);
		FD_SET(ritardatore, &canRead);
		FD_SET(ritardatore, &canWrite);
		FD_SET(ritardatore, &canExcept);
		maxFd = (sender < ritardatore)? ritardatore : sender;
		
		/* Main (and only) point of blocking from now on */
		selectResult = select(maxFd+1, &canRead, &canWrite, &canExcept, NULL);
		
		/* First of all check for errors on sockets */
		if (FD_ISSET(sender, &canExcept)){
			printError("There was an error with the sender socket!");
		}
		if (FD_ISSET(ritardatore, &canExcept)){
			printError("There was an error with the ritardatore socket!");
		}
		
		/* Now check for I/O availability */
		if (FD_ISSET(sender, &canRead)){
			
		}
		if (FD_ISSET(ritardatore, &canRead)){
			
		}
		if (FD_ISSET(ritardatore, &canWrite)){
			
		}
		
		
		
		
		
		
		
		
		continue;
		/* RICEZIONE TCP */
		readCounter = recv(connectedSender, recvBuffer, 1000, 0);
		printf("reading!\n");
		printf("[MGS_TCP]: %s", recvBuffer);
		exit(0);
		memset(recvBuffer, 0, strlen(recvBuffer));
		sleep(3);
		/* INVIO */
		sprintf(sendBuffer, "[MSG] Ciao\n");
		sentCounter = sendto(ritardatore, sendBuffer, strlen(sendBuffer)+1, 0, (struct sockaddr *)&to, sizeof(to));
		/* RICEZIONE */
		sleep(3);
		readCounter = recv(ritardatore, recvBuffer, 1000, MSG_DONTWAIT);
		printf("[MSG]: %s", recvBuffer);
		memset(recvBuffer, 0, strlen(recvBuffer));
	}
}
