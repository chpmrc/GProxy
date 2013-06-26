
/* Network */
struct sockaddr_in toRit, fromRit, dest; /* Indirizzo del socket locale e remoto del ritardatore e del receiver */ 
int ritardatore, receiver; /* socket descriptor verso/da il ritardatore e verso receiver */
socklen_t toLen = sizeof(toRit);
socklen_t destLen = sizeof(dest);

char destAddress[IP_ADDR_STRLEN] = "127.0.0.1",
	fromRitAddress[IP_ADDR_STRLEN] = "0.0.0.0", 
	toRitAddress[IP_ADDR_STRLEN];

unsigned short int ritPorts[] = {62000, 62001, 62002},
					destPort = 64000,
					fromRitPort = 63000,
					toRitPort;



/* Utils */
char recvBuffer[PAYLOAD_SIZE], 
	sendBuffer[PAYLOAD_SIZE];
	
int trueOpt = 1, 
	connectStatus,
	sharedError, /* Shared variables that simulates errno for those functions that don't return a value in case of error */
	recvCounter, sendCounter, /* WARNING: don't use size_t since it's an unsigned numeric type! Not suitable for possible error return values (e.g. -1) */
	counter, /* Generic counter */
	result, /* Used to store temporary return values (e.g. for syscalls) */
	channel = 0; /* Ritardatore's channel currently in use */
	
unsigned int lastSentId = 1; /* 0 is reserved for maintainance! */
	
Node *toSend, *toAck; /* List heads, switch between two lists */
Node *current; /* Used to store temporary nodes */
boolean finalize = FALSE; /* Set to true when the finalizing packet has been received from the ritardatore */
		
/* Profiling */
time_t startTime, endTime;
boolean started = FALSE,
		timedOut = FALSE;


/* Select */
fd_set canRead, canWrite, canExcept, canReadCopy, canWriteCopy, canExceptCopy;
struct timeval timeout = { 5, 0 };
int selectResult;
int maxFd;


/**
 * Init all the variables.
 */
void init(){
	printf("[Call] init\n");
	/* Initialize ports */
	toRitPort = ritPorts[0];
	
	/* Initialize local structures */
	toSend = allocHead();
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
	FD_ZERO(&canRead);
	FD_SET(ritardatore, &canRead);
	maxFd = ritardatore;
}
 
/**
 * Close the used sockets and print profiling info
 */
void terminate(){
	printf("[Call] terminate\n");
	close(ritardatore);
	close(receiver);
	/* Report time */
	time(&endTime);
	/* Remove the timeout seconds if the connection has timed out */
	if (timedOut){
		endTime -= timeout.tv_sec;
	}
	printf("Transfer complete!\nPackets received (size %d): %d | Time elapsed: %f\n", PAYLOAD_SIZE+HEADER_SIZE, lastSentId, difftime(endTime, startTime));
	/* free(recvBuffer);
	free(sendBuffer); */
	exit(0);
}

/**
 * Send an ack for the given packet (id).
 */
void ackPkt(Node *current){
	Pkt ack; /* Used to send ack to the psender */
	printf("[Call] ackPkt\n");
	/* Build and send the ack */
	ack.id = 0;
	ack.type = 'B';
	/* Convert the ID's endianess */
	sprintf(ack.body, "%d%c", htonl(current->packet->id), '\0');
	printf("Sending ack for ID %d\n", current->packet->id);
	/* Since we have to send on a different port we can't use the default binding, sendto is mandatory */
	sendto(ritardatore, &ack, sizeof(Pkt), 0, (struct sockaddr *)&toRit, toLen);
}

/**
 * Send an ack for the given packet and remove it from the toAck list.
 */
void ackPacket(Node *current){
	printf("[Call] ackPacket\n");
	ackPkt(current);
	/* If the last packet has been sent it means the connection is (hopefully) going to terminate soon */
	if (current->packet->id == 0){
		finalize = TRUE;
	}
	removeNode(current);
	clearNode(current);
}


/**
 * If the packet's id is equal to the one we still have to send: 
 * 		send it.
 * If it's lower it means the psender didn't receive the ack: 
 * 		ack it but don't send it to the receiver.
 * In any case ack the packet (we don't need the psender to send it again).
 * If the packet is the final one set the finalize variable to true.
 * @param current The node containing the packet to send
 */
void sendToReceiver(Node *current){
	printf("[Call] sendToReceiver\n");
	if (current != NULL){
		/* NOTICE: the finalizing packet will never be sent cause its ID is always 0 */
		if (current->packet->id == lastSentId){
			sendCounter = send(receiver, current->packet->body, current->pktSize - HEADER_SIZE, 0); /* Only send the body! */
			switch(sendCounter){
				case 0:
					/* Can't happen since the send is blocking */
					break;
					
				case -1:
					printError("There was an error while sending a packet to the receiver");
					break;
					
				default:
					/* The packet has been correctly sent to the receiver. Remove it from the ones to send */
					lastSentId++;
					current = removeNode(current);
					insertNodeById(toAck, current);
			}
		}
	}
}

/**
 * Receive the packet from the ritardatore.
 * If it's type is B:
 * 		Insert the packet in the list of the ones to send/ack keeping the list sorted.
 * If it's type is I:
 * 		It's an ICMP from the ritardatore, switch port.
 */
void receiveFromPsender(){
	
	int maxOps = 100, /* After 100 failed recv it means there is something wrong */
		opsCounter = 0;
		
	printf("[Call] receiveFromPsender\n");
	
	/* Create a dummy packet to store the real one */
	current = allocNode(0, 'B', NULL, 0);
	memset(current->packet, 0, sizeof(Pkt));
	
	/* If the syscall has been interrupted just try again */
	do {
		recvCounter = recv(ritardatore, current->packet, sizeof(Pkt), MSG_DONTWAIT);
		opsCounter++;
	} while (recvCounter < 0 && errno != EINTR && opsCounter < maxOps);
	
	/* Distinguish between body packets and ICMP packets */
	if (recvCounter > 0){

		if (current->packet->type == 'B'){
			
			/* Change the endianess of the ID */
			current->packet->id = ntohl(current->packet->id);
			
			/* We received a packet for the receiver */
			printf("Received ID %d\n", current->packet->id);
			current->pktSize = recvCounter;
			
			/* If the packet has already been received once just ack it */
			if (current->packet->id >= lastSentId){
				/* Here length will be increased */
				insertNodeById(toSend, current);
			} else {
				if (toAck->length < maxPackets){
					insertNodeById(toAck, current);
				} else {
					clearNode(current);
				}
			}
			
		} else {
			/* We received an ICMP from the ritardatore: switch port */
			channel = (channel + 1) % 3;
			toRitPort = ritPorts[channel];
			toRit = getSocketAddress(toRitAddress, toRitPort);
			if (sharedError){
				printError("Failed to create a new address for the socket to the ritardatore");
			}
			printf("Port switched to %d\n", toRitPort);
			clearNode(current);
		}
	} else if (recvCounter < 0 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
		clearNode(current);
		printError("There was an error with recv");
	} else {
		/* Nothing to read, just clear the allocated node */
		printf("Nothing to read from the ritardatore...\n");
		clearNode(current);
	}
}
