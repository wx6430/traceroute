/*
 ============================================================================
 Name        : main.c
 Author      : xwang M201372501
 Copyright   : Your copyright notice
 description : 感觉用python写的太没技术含量了，补充一个Linux C的。
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define IP "115.156.150.44"
#define PORT 27015
#define BUFSIZE 256
#define NUMBER "M201372501"

int receiv(int sock)
{
	char buffer[BUFSIZE];
	int bytes = 0;
	if ((bytes = recv(sock, buffer, BUFSIZE - 1, 0)) < 1)
	{
		printf("Failed to receive bytes from server");
	}
	buffer[bytes] = '\0';
	printf("%s", buffer);
	return 0;
}

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in server;

	/* Create the TCP socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printf("Failed to create socket");
		exit(1);
	}

	/* Construct the server sockaddr_in structure */
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(IP);
	server.sin_port = htons(PORT);
	if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		printf("Failed to connect with server");
		exit(1);
	}

	send(sock, NUMBER, sizeof(NUMBER), 0);
	receiv(sock);
	close(sock);
	return 0;
}
