#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	
	// first call to socket() function
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sockfd < 0)
	{
		perror("ERROR opening socket");
		exit(1);
	}
	
	// initialize socket structure
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = 5001;
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	
	// now bind the host address using bind() call
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR on binding");
		exit(1);
	}
	
	// now start listening for the clients, here process will go in sleep mode and will wait for the incoming connection
	
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);
	
	// accept actual connection from the client
	newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	
	if (newsockfd < 0)
	{
		perror("ERROR on accept");
		exit(1);
	}
	
	// if connection is established then start communicating
	bzero(buffer, 256);
	n = read(newsockfd, buffer, 255);
	
	if (n < 0)
	{
		perror("ERROR reading from socket");
		exit(1);
	}
	
	printf("Here is the message: %s\n", buffer);
	
	// write a response to the client
	n = write(newsockfd, "I got your message", 18);
	
	if (n < 0)
	{
		perror("ERROR writing to socket");
		exit(1);
	}
	
	return 0;
}
