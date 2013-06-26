#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <features.h>

#define TRUE 1
#define FALSE 0
#define SOCKERROR -1
#define HEADER_SIZE 5

/* Debug */
int allocCounter = 0;

/* Shared values (PROTOCOL) */
char endingBody[] = "inanisonoblu";

int sharedError; /* errno-like variable */
char *logFilePath = "./log.txt"; /* Log is also saved to a file */
int firstExecution = TRUE; /* At the beginning do something, then something else */

int maxPackets,
	payload;

/**
 * @param {int} type
 * @return {int}
 * Questa funzione si occupa di ritornare un socket descriptor 
 * creato a seconda del tipo dato come argomento (DGRAM o STREAM)
 */
int getSocket(int type){
	int sockfd;
	int trueOpt = 1;
	sockfd = socket(AF_INET, type, 0);
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (int *)&trueOpt, sizeof(trueOpt)) == SOCKERROR){
		return -1;
	}
	return sockfd;
}

/**
 * @param {char *} ipAddr
 * @param {short int} port
 * @return {struct sockaddr}
 * Questa funzione si occupa di ritornare un socket descriptor 
 * creato a seconda del tipo dato come argomento (DGRAM o STREAM).
 * Ritorna una struct sockaddr_in. address è una stringa in 
 * dotted notation (es. "127.0.0.1").
 */
struct sockaddr_in getSocketAddress(char *address, unsigned short int port){
	struct sockaddr_in newAddr;
	memset(&newAddr, 0, sizeof(newAddr));
	newAddr.sin_family = AF_INET;
	if (!inet_aton(address, &newAddr.sin_addr)){
		sharedError = TRUE;
	}
	newAddr.sin_port = htons(port);
	sharedError = FALSE;
	return newAddr;
}

/**
 * @param {char *} filePath The path to the log file
 */
void logAppend(char* string){
	FILE *debug = fopen(logFilePath, "a+");
	if (firstExecution){
		char currentDate[1000];
		time_t instant = time(NULL);
		/* Viene sempre usata la stessa locazione di memoria ma non ci interessa */
		strftime(currentDate, 1000, "%d/%m/%y - %H:%M:%S", localtime(&instant));
		firstExecution = FALSE;
		fprintf(debug, "%s%s%s", "\n\n==================  ", currentDate ," ==================\n\n");
	}
	fprintf(debug, "%s", string);
	fclose(debug);
}

/**
 * @param {char *} message
 * Questa funzione stampa un messaggio di errore e termina l'esecuzione
 * sfruttando la funzione perror per stampare anche il messaggio relativo
 * al valore di errno.
 */
void printError(char *message){
	char msg[500];
	char errstr[200];
	sprintf(errstr, "%s", strerror(errno));
	if (errno == 0){
		sprintf(errstr, "%s", "This was not an OS related error so no errno value was set!");
	}
	sprintf(msg, "%s%s\n\t%s\n", "[ERROR] ", message, errstr);
	printf("%s", msg);
	exit(1);
}

/**
 * @param {char *} message
 * Questa funzione stampa un messaggio di log sullo stdout.
 */
void printLog(char *message){
	char msg[1000];
	sprintf(msg, "%s%s\n", "[LOG] ", message);
	printf("%s", msg);
	logAppend(msg);
}
