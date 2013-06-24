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


/* Shared */
extern int maxPackets,
			payload;

/* Network */
struct sockaddr_in toRit, /* Address structures */ 
		fromRit, 
		source, 
		sourceClient; 
		
int ritardatore, /* Socket descriptors */
	sender, 
	connectedSender; 
	
unsigned short int ritPorts[] = {61000, 61001, 61002}, /* Ports used by the Ritardatore */
					sourcePort = 59000, /* Address:port from the sender */
					fromRitPort = 60000, /* Address:port pair from the ritardatore */
					toRitPort; /* Address:port pair to the ritardatore */
					
char sourceAddress[] = "0.0.0.0",
	fromRitAddress[] = "0.0.0.0",
	toRitAddress[] = "192.168.1.125";
	

socklen_t toLen = sizeof(toRit);
socklen_t sourceClientLen = sizeof(sourceClient);

/* Data structures */
char *recvBuffer;
Node *toSend, *current, *endingNode;
Pkt currentAck;

/* Utils */
boolean finalize = FALSE, /* Set to true when the finalizing packet has been sent to the ritardatore */
		endingRead = FALSE; /* Set to true when there is no data left from the sender */
		
int sharedError; /* Shared variables that simulates errno for those functions that don't return a value in case of error */
int readCounter, /* Counter of the packets read from the sender each time */
	sentCounter, /* Counter of the packets sent to the Ritardatore each time */
	currentId = 1, /* 0 is reserved for maintainance! */
	counter, /* Generic counter */
	result, /* Used to store temporary return values (e.g. for syscalls) */
	channel = 0; /* Ritardatore's channel currently in use */
	
/* Select utils */
fd_set canRead, canReadCopy;
struct timeval timeout = { 10, 0 };
int selectResult;
int maxFd;

/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */

/**
 * Functions headers
 */

void closeSenderConnection();

void sendPacket(Node *node);

void receiveFromSender();

void checkRitardatore();

void init();

void terminate();

/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */

/**
 * Main flow
 */

int main(int argc, char *argv[]){
	printf("[Call] main\n");
	
	/* Initialize the connection parameters */
	if (argc < 3){
		printf("Usage: psender RITARDATORE_ADDRESS MAX_PACKETS\n");
		exit(1);
	} else {
		strcpy(toRitAddress, argv[1]);
		/*payload = atoi(argv[3]);*/
		payload = PAYLOAD_SIZE;
		maxPackets = atoi(argv[2]);
		recvBuffer = (char *)malloc(sizeof(char)*payload);
		if ((int)recvBuffer == -1){
			printError("There was an error with malloc()\n");
		}
	}
	
	/* Initialize all the variables */
	init();
	
	while(TRUE){

		/* Make a copy of the fd_set toRit avoid modifying the original one */
		canReadCopy = canRead;
		
		/* Main (and only) point of blocking fromRit now on */
		selectResult = select(maxFd+1, &canReadCopy, NULL, NULL, &timeout);
		
		/* Check for errors */
		if (selectResult < 0){
			printError("There was an error with the select function!");
		} else 

		/* Check for active sockets */
		if (selectResult > 0){
		
			/* Check if we can receive data from the sender */
			if (FD_ISSET(connectedSender, &canReadCopy)){
				while (toSend->length < maxPackets && !endingRead){
					receiveFromSender();
				}
			}
			
			/* Here we have to scan the list and send its packets to get the related ack */
			forEach(toSend, &sendPacket, maxPackets);
			
			/* Check if we can receive data fromRit the ritardatore */
			if (FD_ISSET(ritardatore, &canReadCopy)){
				/* Try to get an ack for each packet in the list */
				counter = 0;
	
				while (counter < maxPackets){
					checkRitardatore();
					counter++;
				}
				
			}
			
			/* Check if we sent all the packets */
			if (toSend->length == 0 && finalize){
				terminate();
			}
			
		} else if (selectResult == 0){
			/* If the connection timed out just terminate */
			terminate();
		}
	}
}

/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */

/** 
 * Functions implementation 
 */
 

/**
 * Close all the used sockets and exit
 */
void terminate(){
	printf("[Call] terminate\n");
	printf("Terminating connection...\n");
	closeSenderConnection();
	close(ritardatore);
	free(recvBuffer);
	exit(0);
}


/**
 * Unset and close the connected sender socket so that it won't be
 * longer used in the select.
 */
void closeSenderConnection(){
	printf("[Call] closeSenderConnection\n");
	FD_CLR(connectedSender, &canRead);
	maxFd = ritardatore;
	close(connectedSender);
	close(sender);
}

/**
 * Read some data from the sender.
 * Store the data as the body of a newly allocated node.
 * Insert the node in the list of the packets to send.
 */
void receiveFromSender(){
	printf("[Call] receiveFromSender\n");

	/* Fill the buffer */
	readCounter = recv(connectedSender, recvBuffer, payload, 0);
	/* Check cases */
	switch(readCounter){
		case 0:
			/* We finished reading data from the sender, now we only have to transmit it */
			endingNode = allocNode(0, 'B', endingBody, strlen(endingBody)+HEADER_SIZE+1 /* trailing 0 */);
			appendNode(toSend, endingNode);
			endingRead = TRUE;
			closeSenderConnection();
			break;
			
		case -1:
			printError("There was an error while reading data from the sender");
			break;
			
		default:
			/* Build a node and append it to the toSend list */
			current = allocNode(currentId, 'B', recvBuffer, readCounter+HEADER_SIZE);
			appendNode(toSend, current);
			currentId++;
	}				
}

/**
 * Read a packet from the ritardatore:
 * 	- If the packet is type B it's an ack:
 * 		- Remove the corresponding node from the toSend list.
 * 	- If the packet is type I it's an ICMP from the ritardatore:
 * 		- Switch port.
 */
void checkRitardatore(){
	printf("[Call] checkRitardatore\n");
	
	readCounter = recv(ritardatore, &currentAck, sizeof(Pkt), MSG_DONTWAIT);
	/* Check the type of the packet */
	if (currentAck.type == 'B'){
		/* If we receive a body packet it must be an ack from the preceiver */
		current = removeNodeById(toSend, atoi(currentAck.body));
		clearNode(current);
	} else {
		/* Switch port */
		channel = (channel + 1) % 3;
		toRitPort = ritPorts[channel];
		toRit = getSocketAddress(toRitAddress, toRitPort);
		if (sharedError){
			printError("creazione dell'indirizzo associato al socket verso il ritardatore fallita! Errore");
		}
		printf("Port switched to %d\n", toRitPort);
	}
}

/**
 * Send the given packet to the ritardatore.
 * Also check if the finalizing packet has been reached.
 */
void sendPacket(Node *node){
	Pkt *pkt = node->packet;
	printf("[Call] sendPacket\n");
	sentCounter = sendto(ritardatore, pkt, node->pktSize, 0, (struct sockaddr *)&toRit, sizeof(toRit));

	if (sentCounter < 0){
		printError("There was an error with sendto()");
	} else {
		printf("Sent ID %d\n", pkt->id);
		/* If the finalizing packet has been sent set finalize to true */
		if (pkt->id == 0){
			finalize = TRUE;
		}
	}
}

void init(){
	printf("[Call] init\n");
	/* Initialize ports */
	toRitPort = ritPorts[channel];
	
	/* Initialize local structures */
	toSend = allocHead();
	
	/* Creo il socket descriptor per il ritardatore e ne setto le specifiche opzioni */
	if ((ritardatore = getSocket(SOCK_DGRAM)) == SOCKERROR){
		printError("Failed to create socket to/from ritardatore");
	}
	
	/* Creo il socket descriptor per la sorgente/sender */
	if ((sender = getSocket(SOCK_STREAM)) == SOCKERROR){
		printError("Failed to create a socket to get data from the sender");
	}
	
	/* Inizializzo la struttura per il socket in ricezione dal sender/sorgente */
	source = getSocketAddress(sourceAddress, sourcePort);
	if (sharedError){
		printError("Failed to create an address structure for the socket from the sender");
	}
	printf("Sender socket address: %s:%d\n", sourceAddress, sourcePort);
	
	/* Inizializzo la struttura per il socket in ricezione dal ritardatore */
	fromRit = getSocketAddress(fromRitAddress, fromRitPort);
	if (sharedError){
		printError("Failed to create an address structure for the socket from the ritardatore");
	}
	printf("Ritardatore (from) socket address: %s:%d\n", fromRitAddress, fromRitPort);
	
	/* Inizializzo la struttura per il socket in invio verso il ritardatore */
	toRit = getSocketAddress(toRitAddress, toRitPort);
	if (sharedError){
		printError("Failed to create an address structure for the socket to the ritardatore");
	}
	printf("Ritardatore (to) socket address: %s:%d\n", toRitAddress, toRitPort);
	
	/* BINDS AND LISTENS */
	
	/* Lego il socket dal ritardatore ad un indirizzo in ricezione */
	result = bind(ritardatore, (struct sockaddr *)&fromRit, sizeof(fromRit));
	if (result == SOCKERROR){
		printError("Bind failed for the socket from the ritardatore");
	}
	
	/* Lego il socket dal sender ad un indirizzo in ricezione */
	result = bind(sender, (struct sockaddr *)&source, sizeof(source));
	if (result == SOCKERROR){
		printError("Bind failed for the socket from the ritardatore");
	}
	
	/* Listen for connections from the sender */
	result = listen(sender, 1);
	if (result == SOCKERROR){
		printError("Call to listen for connections from the sender failed");
	}
	
	/* First of all estabilish a connection with the source (only one connection at a time is allowed) */
	connectedSender = accept(sender, (struct sockaddr *)&sourceClient, &sourceClientLen);
	if (connectedSender < 0){
		printError("Accepting connection from the sender failed");
	}
	printf("Client connected on port: %d\n", ntohs(sourceClient.sin_port));
	
	/* Add the sockets into the sets of descriptors */
	FD_ZERO(&canRead);
	FD_SET(connectedSender, &canRead); /* Listen on the connected socket */
	FD_SET(ritardatore, &canRead);
	maxFd = (connectedSender < ritardatore)? ritardatore : connectedSender;
}
