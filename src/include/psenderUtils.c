
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
					
char sourceAddress[IP_ADDR_STRLEN] = "0.0.0.0",
	fromRitAddress[IP_ADDR_STRLEN] = "0.0.0.0",
	toRitAddress[IP_ADDR_STRLEN];
	

socklen_t toLen = sizeof(toRit);
socklen_t sourceClientLen = sizeof(sourceClient);

/* Data structures */
char recvBuffer[PAYLOAD_SIZE];
Node *toSend, *current, *endingNode;
Pkt currentAck;

/* Utils */
boolean finalize = FALSE, /* Set to true when the finalizing packet has been sent to the ritardatore */
		endingRead = FALSE; /* Set to true when there is no data left from the sender */
		
int sharedError; /* Shared variables that simulates errno for those functions that don't return a value in case of error */
int counter, /* Generic counter */
	result, /* Used to store temporary return values (e.g. for syscalls) */
	channel = 0; /* Ritardatore's channel currently in use */
	
unsigned int currentId = 1; /* 0 is reserved for maintainance! */
	
/* Select utils */
fd_set canRead, canReadCopy;
struct timeval origTimeout = { 10, 0 },
				actualTimeout;
int selectResult;
int maxFd;

/** 
 * Init all the variables.
 */
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
 * Close all the used sockets and exit
 */
void terminate(){
	printf("[Call] terminate\n");
	printf("Terminating connection...\n");
	closeSenderConnection();
	close(ritardatore);
	/* free(recvBuffer); */
	exit(0);
}


/**
 * Read some data from the sender.
 * Store the data as the body of a newly allocated node.
 * Insert the node in the list of the packets to send.
 */
void receiveFromSender(){

	/* Fill the buffer */
	int readCounter;
	
	printf("[Call] receiveFromSender\n");
	
	/* Execute the (blocking) recv while the error is just a OS interruption */
	do {
		readCounter = recv(connectedSender, recvBuffer, PAYLOAD_SIZE, 0);
	} while (readCounter < 0 && errno != EINTR);

	/* Check recv cases */
	switch(readCounter){
		case 0:
			/* We finished reading data from the sender, now we only have to transmit it */
			endingNode = allocNode(0, 'B', endingBody, strlen(endingBody)+HEADER_SIZE+1 /* string plus '\0' */);
			appendNode(toSend, endingNode);
			endingRead = TRUE;
			closeSenderConnection();
			break;
			
		case -1:
			/* If the syscall has been interrupted just gracefully exit and wait for the next call */
			if (errno != EINTR){
				printError("There was an error while reading data from the sender");
			}
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
	
	/* Read a packet */
	int readCounter = recv(ritardatore, &currentAck, sizeof(Pkt), MSG_DONTWAIT);
	
	printf("[Call] checkRitardatore\n");
	
	if (readCounter > 0){
		/* Check the type of the packet */
		if (currentAck.type == 'B'){
			
			/* If we receive a body packet it must be an ack from the preceiver */
			current = removeNodeById(toSend, ntohl(atoi(currentAck.body)));
			printf("Acking ID %d\n", ntohl(atoi(currentAck.body)));
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
		
	} else if (readCounter < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
		printError("There was an error while receiving packets from the ritardatore\n");
	}
}

/**
 * Send the given packet to the ritardatore.
 * Also check if the finalizing packet has been reached.
 */
void sendPacket(Node *node){
	
	Pkt *pkt = node->packet;
	int sentCounter;
	
	/* Convert to internet endianess on the fly */
	pkt->id = htonl(pkt->id);
	
	/* Send the packet's body */
	do {
		sentCounter = sendto(ritardatore, pkt, node->pktSize, 0, (struct sockaddr *)&toRit, sizeof(toRit));
	
	} while (sentCounter < 0 && (errno == EINTR || errno == EAGAIN));
	
	printf("[Call] sendPacket\n");

	if (sentCounter < 0){
		printError("There was an error with sendto()");
	} else {
		/* If the finalizing packet has been sent set finalize to true */
		if (pkt->id == 0){
			finalize = TRUE;
		}
	}
	
	/* Convert back to host endianess */
	pkt->id = ntohl(pkt->id);
	
	printf("Sent ID %d\n", pkt->id);
}
