#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <string.h>

#define RCVBUFSIZE 256   /* Size of receive buffer */

void DieWithError(char *errorMessage);  /* Error handling function */

char* ReadHTTPRequest(const char* request) {
    if (strncmp(request, "GET", 3) != 0) DieWithError("Unknown HTTP request");

    // find get request path
    const char* pathStart = strchr(request, ' ');

    if (pathStart != NULL) {
        pathStart++; // skip the space
        const char* pathEnd = strchr(pathStart, ' ');
        if (pathEnd != NULL) {
            int len = pathEnd - pathStart;
            char* extractedPath = malloc(sizeof(char) * (len + 1));
            extractedPath[len] = '\0';
            strncpy(extractedPath, pathStart, len);
            return extractedPath;
        } else {
            DieWithError("Parsed path too long");
        }
    } else {
        DieWithError("Unable to parse requested path");
    }
    return NULL; // appease the compiler warnings
}

void SendHTTPResponse(const char* path, const char* root, int clientSocket) {
    const char* okResponse = "HTTP/1.0 200 OK\r\n";
    const char* notFoundResponse = "HTTP/1.0 404 Not Found\r\n";

    const char* htmlContentType = "Content-Type: text/html\r\n\r\n";
    const char* textContentType = "Content-Type: text/plain\r\n\r\n";
    const char* jpegContentType = "Content-Type: image/jpeg\r\n\r\n";
    const char* gifContentType = "Content-Type: image/gif\r\n\r\n";
    const char* nullContentType = "\r\n";

    const char** selectedContentType = &nullContentType;

    char* requestedFileName;
    FILE* requestedFile;

    // redirect / to index.html
    if (strcmp(path, "/") == 0) path = "/index.html";

    // detect the content type from file extension (don't verify the file is actually that type)
    const char* lastDot = strrchr(path, '.');
    lastDot++;

    if (strcmp(lastDot, "html") == 0 || strcmp(lastDot, "htm") == 0) {
        selectedContentType = &htmlContentType;
    } else if (strcmp(lastDot, "txt") == 0) {
        selectedContentType = &textContentType;
    } else if (strcmp(lastDot, "jpg") == 0 || strcmp(lastDot, "jpeg") == 0) {
        selectedContentType = &jpegContentType;
    } else if (strcmp(lastDot, "gif") == 0) {
        selectedContentType = &gifContentType;
    }

    // make file path
    if (strstr(path, "..") != NULL) { // Verify it's a subdirectory
        printf("ERROR: attempted traversal out of root\n");
        return;
    } else {
        int filenameLength = strlen(path) + strlen(root) + 1;
        requestedFileName = malloc(sizeof(char) * (filenameLength));
        strncpy(requestedFileName, root, strlen(root));
        strncpy(requestedFileName+strlen(root), path, strlen(path));
        requestedFileName[filenameLength] = '\0';
        printf("constructed file name: %s\n", requestedFileName);
    }

    // open file
    requestedFile = fopen(requestedFileName, "r");
    free(requestedFileName);

    // read and send the file
    if (requestedFile == NULL) {
        printf("ERROR: couldn't open file\n");
        send(clientSocket, notFoundResponse, sizeof(char)*strlen(notFoundResponse), 0);
        send(clientSocket, nullContentType, sizeof(char)*strlen(nullContentType), 0);
        return;
    } else {
        char readBuffer[100];
        send(clientSocket, okResponse, sizeof(char)*strlen(okResponse), 0);
        send(clientSocket, *selectedContentType, sizeof(char)*strlen(*selectedContentType), 0);
        int readCount = 0;
        do {
            readCount = fread(readBuffer, sizeof(char), 100, requestedFile);
            send(clientSocket, readBuffer, readCount*sizeof(char), 0);
        } while (readCount == 100);
    }

    fclose(requestedFile);
}

void HandleHTTPClient(int clntSocket, const char* root)
{
    char requestBuffer[RCVBUFSIZE];        /* Buffer for echo string */
    int requestMessageSize;                    /* Size of received message */

    /* Receive message from client */
    if ((requestMessageSize = recv(clntSocket, requestBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    printf("*** serving request ***\n%.256s\n*** --------------- ***\n", requestBuffer);

    char* requestedPath = ReadHTTPRequest(requestBuffer);

    printf("REQUESTED PATH: %s\n", requestedPath);

    SendHTTPResponse(requestedPath, root, clntSocket);

    close(clntSocket);    /* Close client socket */
    free(requestedPath);
}
