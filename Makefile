
.PHONY: all, clean

all: HTTPServer

clean:
	rm HTTPServer
	rm -rf ./*.dSYM

HTTPServer:
	gcc -g -o HTTPServer HTTPServer-Fork.c DieWithError.c AcceptTCPConnection.c CreateTCPServerSocket.c HandleHTTPClient.c