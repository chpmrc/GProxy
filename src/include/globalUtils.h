#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0
#define SOCKERROR -1

int sharedError; /* errno-like variable */

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
 * @param {char *} message
 * Questa funzione stampa un messaggio di errore e termina l'esecuzione
 * sfruttando la funzione perror per stampare anche il messaggio relativo
 * al valore di errno.
 */
void printError(char *message){
	char msg[1000] = "[ERROR] ";
	if (strlen(message) > 1000){
		printf("[ERROR] overflow con strcat durante la creazione di un messaggio di errore!");
		exit(2);
	}
	strcat(msg, message);
	printError(msg);
	exit(1);
}

/**
 * @param {char *} message
 * Questa funzione stampa un messaggio di log sullo stdout.
 */
void printLog(char *message){
	printf("[LOG] %s\n", message);
}
