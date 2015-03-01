#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define MAX_PENDING_REQUESTS 5    /* Maximum outstanding connection requests */

void DieWithError(char *errorMessage);  /* Error handling function */
void HandleHTTPClient(int clntSocket, const char* root);   /* TCP client handling function */

int main(int argc, char *argv[])
{
    int serverSocket;                    /* Socket descriptor for server */
    int clientSocket;                    /* Socket descriptor for client */
    struct sockaddr_in HTTPServerSocketAddress; /* Local address */
    struct sockaddr_in HTTPClientSocketAddress; /* Client address */
    unsigned short HTTPServerPort;     /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */

    if (argc != 3)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port> <Server Root Path>\n", argv[0]);
        exit(1);
    }

    HTTPServerPort = atoi(argv[1]);  /* First arg:  local port */


    /* Create socket for incoming connections */
    if ((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");
      
    /* Construct local address structure */
    memset(&HTTPServerSocketAddress, 0, sizeof(HTTPServerSocketAddress));   /* Zero out structure */
    HTTPServerSocketAddress.sin_family = AF_INET;                /* Internet address family */
    HTTPServerSocketAddress.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    HTTPServerSocketAddress.sin_port = htons(HTTPServerPort);      /* Local port */

    /* Bind to the local address */
    if (bind(serverSocket, (struct sockaddr *) &HTTPServerSocketAddress, sizeof(HTTPServerSocketAddress)) < 0)
        DieWithError("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(serverSocket, MAX_PENDING_REQUESTS) < 0)
        DieWithError("listen() failed");

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(HTTPClientSocketAddress);

        /* Wait for a client to connect */
        if ((clientSocket = accept(serverSocket, (struct sockaddr *) &HTTPClientSocketAddress, 
                               &clntLen)) < 0)
            DieWithError("accept() failed");

        /* clientSocket is connected to a client! */

        printf("Handling client %s\n", inet_ntoa(HTTPClientSocketAddress.sin_addr));

        HandleHTTPClient(clientSocket, argv[2]);
    }
    /* NOT REACHED */
}
