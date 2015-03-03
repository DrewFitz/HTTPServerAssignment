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
        const char* pathEnd = strchr(pathStart, ' '); // find the next space
        if (pathEnd != NULL) {
            int len = pathEnd - pathStart; // find distance between two pointers
            char* extractedPath = malloc(sizeof(char) * (len + 1)); // alloc new string
            extractedPath[len] = '\0'; // write trailing null character
            strncpy(extractedPath, pathStart, len); // copy string
            return extractedPath;
        } else {
            // if there was no second space in this string, it didn't fit in the request buffer
            DieWithError("Parsed path too long"); 
        }
    } else {
        // something is unexpected with the formatting of the request
        DieWithError("Unable to parse requested path");
    }
    return NULL; // appease the compiler warnings
}

void SendHTTPResponse(const char* path, const char* root, int clientSocket) {
    // the two responses we can send
    const char* okResponse = "HTTP/1.0 200 OK\r\n";
    const char* notFoundResponse = "HTTP/1.0 404 Not Found\r\n";

    // the four content types we return, plus an empty one for 404 and unsupported types
    const char* htmlContentType = "Content-Type: text/html\r\n\r\n";
    const char* textContentType = "Content-Type: text/plain\r\n\r\n";
    const char* jpegContentType = "Content-Type: image/jpeg\r\n\r\n";
    const char* gifContentType = "Content-Type: image/gif\r\n\r\n";
    const char* nullContentType = "\r\n";

    // will point to the detected content type
    const char** selectedContentType = &nullContentType;

    // used for opening the requested file
    char* requestedFileName;
    FILE* requestedFile;

    // redirect / to index.html
    if (strcmp(path, "/") == 0) path = "/index.html";

    // detect the content type from file extension (don't verify the file is actually that type)
    const char* lastDot = strrchr(path, '.'); // put the pointer at the last '.'
    lastDot++; // move it past the last '.'

    // check the file endings of the requested path to select the content-type
    if (strcmp(lastDot, "html") == 0 || strcmp(lastDot, "htm") == 0) {
        selectedContentType = &htmlContentType;
    } else if (strcmp(lastDot, "txt") == 0) {
        selectedContentType = &textContentType;
    } else if (strcmp(lastDot, "jpg") == 0 || strcmp(lastDot, "jpeg") == 0) {
        selectedContentType = &jpegContentType;
    } else if (strcmp(lastDot, "gif") == 0) {
        selectedContentType = &gifContentType;
    } // else it is already set to the "null content type"

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
    free(requestedFileName); // the filename was copied from the input dynamically

    // read and send the file
    if (requestedFile == NULL) {
        // 404 NOT FOUND, couldn't find the file
        printf("ERROR: couldn't open file\n");
        send(clientSocket, notFoundResponse, sizeof(char)*strlen(notFoundResponse), 0);
        send(clientSocket, nullContentType, sizeof(char)*strlen(nullContentType), 0);
        return;
    } else {
        // 200 OK, here's the file
        char readBuffer[100];
        // Send our headers
        send(clientSocket, okResponse, sizeof(char)*strlen(okResponse), 0);
        send(clientSocket, *selectedContentType, sizeof(char)*strlen(*selectedContentType), 0);
        // write the file to the socket 100 bytes at a time
        int readCount = 0;
        do {
            readCount = fread(readBuffer, sizeof(char), 100, requestedFile);
            send(clientSocket, readBuffer, readCount*sizeof(char), 0);
        } while (readCount == 100); // if we don't read 100 bytes, assume we hit EOF
    }

    // clean up after yourself, were you raised in a barn? jeez...
    fclose(requestedFile);
}

void HandleHTTPClient(int clntSocket, const char* root)
{
    char requestBuffer[RCVBUFSIZE]; /* Buffer for http request */
    int requestMessageSize;         /* Size of received message */

    /* Receive message from client */
    // also don't bother with GET requests larger than 256 bytes
    if ((requestMessageSize = recv(clntSocket, requestBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    // debug output
    printf("*** serving request ***\n%.256s\n*** --------------- ***\n", requestBuffer);

    // parse the requested path out of the HTTP request
    char* requestedPath = ReadHTTPRequest(requestBuffer);

    // more debug output
    printf("REQUESTED PATH: %s\n", requestedPath);

    // attempt to open the file and send back the contents
    SendHTTPResponse(requestedPath, root, clntSocket);

    close(clntSocket);    /* Close client socket */
    free(requestedPath); // this string was dynamically allocated
}
