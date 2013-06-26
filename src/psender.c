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
#include "include/psenderUtils.c"


/* Shared */
extern int maxPackets;

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
		maxPackets = atoi(argv[2]);
		if ((int)recvBuffer == -1){
			printError("There was an error with malloc()\n");
		}
	}
	
	/* Initialize all the variables */
	init();
	
	while(TRUE){

		/* Make a copy of the fd_set toRit avoid modifying the original one */
		canReadCopy = canRead;
		
		/* Here we have to scan the list and send its packets to get the related ack */
		forEach(toSend, &sendPacket, maxPackets); /* Can't do this after the select cause it might block! */

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
				
				/* Read packets unless we fill the list */
				while (toSend->length < maxPackets && !endingRead){
					receiveFromSender();
				}
			}
			
			/* Check if we can receive data fromRit the ritardatore */
			if (FD_ISSET(ritardatore, &canReadCopy)){
				
				/* Try to get many ack */
				for (counter = 0; counter < maxPackets; counter++){
					checkRitardatore();
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
