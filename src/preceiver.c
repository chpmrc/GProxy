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
#include "include/preceiverUtils.c"


/* Shared */
extern int maxPackets,
			allocCounter;

/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */

/**
 * Main flow
 */

int main(int argc, char *argv[]){
	printf("[Call] main\n");
	
	/* Initialize the connection parameters */
	if (argc < 3){
		printf("Usage: preceiver RITARDATORE_ADDRESS MAX_PACKETS\n");
		exit(1);
	} else {
		strcpy(toRitAddress, argv[1]);
		maxPackets = atoi(argv[2]);
		if ((int)recvBuffer == -1 || (int)sendBuffer == -1){
			printError("There was an error with malloc()\n");
		}
	}
	
	/* Initialize all the variables */
	init();
	
	while(TRUE){
		
		/* Make a copy of the fd_set toRit avoid modifying the original one (struct copy) */
		canReadCopy = canRead;
		canWriteCopy = canWrite;
		
		/* If both the lists are empty and finalize is true, terminate */
		if (finalize && toSend->length == 0 && toAck->length == 0){
			terminate();
		}
		
		/* Try to send the collected packets to the receiver */
		forEach(toSend, &sendToReceiver, maxPackets);
		
		/* Ack the already sent packets */
		forEach(toAck, &ackPacket, maxPackets);
		
		/* Reinitialize timeout, struct copy */
		actualTimeout = origTimeout;
		
		/* Main (and only) point of blocking from now on */
		selectResult = select(maxFd+1, &canReadCopy, NULL, NULL, &actualTimeout);
		
		/* Check for errors */
		if (selectResult < 0){
			/* Check if the syscall has been interrupted */
			if (errno != EINTR){
				printError("There was an error with the select function!");
			}
		} else
		
		/* Check for active sockets */
		if (selectResult > 0){
			
			/* Compute transmission time */
			if (!started){
				time(&startTime);
				started = TRUE;
			}
			
			/* Check if we can read data from the ritardatore */
			if (FD_ISSET(ritardatore, &canReadCopy)){
				/* Try to read maxPackets */
				for (counter = 0; counter <= maxPackets; counter++){
					receiveFromPsender();
				}
			}
		} else {
			
			/* Timeout expired, maybe the psender is dead,
			 * just finish sending ack and packets to receiver and terminate */
			timedOut = TRUE;
		}
	}
}
