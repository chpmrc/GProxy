#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

struct sockaddr_in to, from;
int ritardatore;
int OptVal = 1;
char toAddress[] = "192.168.1.76";
char buffer[] = "messaggio\n";

int main (int argc, char *argv[]){
	/* Creazione socket descriptor ritardatore */
	ritardatore = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(ritardatore, SOL_SOCKET, SO_REUSEADDR, (int *)&OptVal, sizeof(OptVal));
	
	/* local */
	memset(&from, 0, sizeof(from));
	from.sin_family = AF_INET;
	from.sin_addr.s_addr = htonl(INADDR_ANY);
	from.sin_port = htons(61000);

	//bind(ritardatore, (struct sockaddr *)&from, sizeof(from));

	/* destinatario */
	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	inet_aton(toAddress, &to.sin_addr);
	to.sin_port = htons(60000);

	sendto(ritardatore, buffer, strlen(buffer)+1, 0, (struct sockaddr *)&to, sizeof(to));

	
}
	
