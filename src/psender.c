/** TODO:
	1) Aprire socket UDP e leggere il contenuto inviando un ACK
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#ifndef __USE_MISC
	#define __USE_MISC /* Avoid warnings on the non standard inet_aton */
#endif	
#define TRUE 1
#define FALSE 0

struct sockaddr_in to, from; /* Indirizzo del socket locale e del ritardatore */ 
int ritardatore; /* socket descriptor verso/da il ritardatore */
char toAddress[] = "127.0.0.1";

/* UTILS */
char recvBuffer[1000];
int trueOpt = 1;
int readCounter;
socklen_t toLen = sizeof(to);

int main(){
	/* Creo il socket descriptor per il ritardatore e ne setto le specifiche opzioni */
	ritardatore = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(ritardatore, SOL_SOCKET, SO_REUSEADDR, (int *)trueOpt, sizeof(trueOpt));
	/* Inizializzo la struttura per il socket in ricezione dal ritardatore */
	memset(&from, 0, sizeof(from));
	from.sin_family = AF_INET;
	from.sin_addr.s_addr = htonl(INADDR_ANY);
	from.sin_port = htons(60000);
	/* Inizializzo la struttura per il socket in invio verso il ritardatore */
	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	inet_aton(toAddress, &to.sin_addr);
	to.sin_port = htons(61000);	
	/* Lego il socket ad un indirizzo in ricezione */
	bind(ritardatore, (struct sockaddr *)&from, sizeof(from));
	/* Leggo i datagram */
	while(TRUE){
		readCounter = recv(ritardatore, recvBuffer, 1000, 0);
		printf("[MSG]: %s", recvBuffer);
	}
}
