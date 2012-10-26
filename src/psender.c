#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "include/globalUtils.h"

struct sockaddr_in to, from, source, sourceClient; /* Indirizzo del socket locale e remoto del ritardatore e del sender */ 
int ritardatore, sender, connectedSender; /* socket descriptor verso/da il ritardatore e dal sender */
char toAddress[] = "127.0.0.1";
char sourceAddress[] = "0.0.0.0";
char fromAddress[] = "0.0.0.0"; /* INADDR_ANY */

/* UTILS */
char recvBuffer[1000], sendBuffer[1000];
int trueOpt = 1;
int sharedError; /* Variabile di riconoscimento errori condivisa, serve per quelle funzioni che non ritornano interi */
int readCounter, sentCounter;
socklen_t toLen = sizeof(to);
socklen_t sourceClientLen = sizeof(sourceClient);


int main(){
	/* INIT */
	
	/* Creo il socket descriptor per il ritardatore e ne setto le specifiche opzioni */
	if ((ritardatore = getSocket(SOCK_DGRAM)) == SOCKERROR){
		printError("[ERROR] creazione fallita per il socket da/verso il ritardatore! Errore");
	}
	printf("rit desc : %d\n", ritardatore);
	printLog("socket da/verso il ritardatore correttamente creato!");
	/* Creo il socket descriptor per la sorgente/sender */
	if ((sender = getSocket(SOCK_STREAM)) == SOCKERROR){
		printError("[ERROR] creazione fallita per il socket dal sender! Errore");
	}
	printLog("socket dal sender correttamente creato!");
	printf("senderDesc : %d\n", sender);
	
	/* Inizializzo la struttura per il socket in ricezione dal sender/sorgente */
	source = getSocketAddress(sourceAddress, 59000);
	if (sharedError){
		printError("[ERROR] creazione dell'indirizzo associato al socket dal sender fallita! Errore");
	}
	printLog("indirizzo associato al socket dal sender correttamente creato!");
	/* Inizializzo la struttura per il socket in ricezione dal ritardatore */
	from = getSocketAddress(fromAddress, 60000);
	if (sharedError){
		printError("[ERROR] creazione dell'indirizzo associato al socket dal ritardatore fallita! Errore");
	}
	printLog("indirizzo associato al socket dal ritardatore correttamente creato!");
	/* Inizializzo la struttura per il socket in invio verso il ritardatore */
	to = getSocketAddress(toAddress, 61000);
	if (sharedError){
		printError("[ERROR] creazione dell'indirizzo associato al socket verso il ritardatore fallita! Errore");
	}
	printLog("indirizzo associato al socket verso il ritardatore correttamente creato!");
	
	/* BINDS AND LISTENS */
	
	/* Lego il socket dal ritardatore ad un indirizzo in ricezione */
	bind(ritardatore, (struct sockaddr *)&from, sizeof(from));
	/* Lego il socket dal sender ad un indirizzo in ricezione */
	bind(sender, (struct sockaddr *)&source, sizeof(source));
	listen(sender, 1);
	
	/* Leggo i datagram */
	while(TRUE){
		/* RICEZIONE TCP */
		
		connectedSender = accept(sender, (struct sockaddr *)&sourceClient, &sourceClientLen);
		printf("accepted!\n");
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
