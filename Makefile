
.PHONY: all, clean

all: HTTPServer

HTTPServer:
	gcc -g -o HTTPServer HTTPServer-Fork.c DieWithError.c AcceptTCPConnection.c CreateTCPServerSocket.c HandleHTTPClient.c