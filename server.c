#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>

#define BYTES 1024

void error(char *);
int startServer(char *port);
void respond(int connfd, char *root);

int main(int argc, char* argv[]) {

	struct sockaddr_in cli_addr;

	//Parsing the command line arguments
	char *port, *root;
	if (argc == 3) {
		port = argv[1];
		root = argv[2];
	} else {
		fprintf(stderr, "USAGE: ./server [port number] [path to web root]\n");
		exit(1);
	}

	int listenfd = startServer(port), connfd;

	printf("HTTP server listening on port %d\n", atoi(port));
	// ACCEPT connections
	while (1) {

		connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);

		if (connfd<0)
			error("accept() error");
		else {
			if (fork()==0) {
				respond(connfd, root);
				exit(0);
			}
		}
	}
	return 0;
}

//start server
int startServer(char *port) {

	struct addrinfo hints, *res, *p;
	int listenfd = -1;

	// getaddrinfo for host
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &hints, &res) != 0) {
		perror ("getaddrinfo() error");
		exit(1);
	}
	// socket and bind
	for (p = res; p != NULL; p = p->ai_next) {
		listenfd = socket (p->ai_family, p->ai_socktype, 0);
		if (listenfd == -1) continue;
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
	}
	if (p == NULL) {
		perror ("socket() or bind()");
		exit(1);
	}

	freeaddrinfo(res);

	// listen for incoming connections
	if (listen (listenfd, 1000000) != 0) {
		perror("listen() error");
		exit(1);
	}
	return listenfd;
}

//client connection
void respond(int connfd, char *root) {
	char mesg[99999], *reqline[3], buf[BYTES], path[99999];
	int rcvd, fd, nbytes;

	memset((void*)mesg, (int)'\0', 99999);

	rcvd=recv(connfd, mesg, 99999, 0);

	if (rcvd<0)    // receive error
		fprintf(stderr,("recv() error\n"));
	else if (rcvd==0)    // receive socket closed
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	else {    // message received
		printf("%s", mesg);
		reqline[0] = strtok (mesg, " \t\n");
		if (strncmp(reqline[0], "GET\0", 4)==0) {
			reqline[1] = strtok (NULL, " \t");
			reqline[2] = strtok (NULL, " \t\n");
			if (strncmp(reqline[2], "HTTP/1.0", 8) != 0 && strncmp( reqline[2], "HTTP/1.1", 8) != 0) {
				write(connfd, "HTTP/1.0 400 Bad Request\n", 25);
			} else {
				if (strncmp(reqline[1], "/\0", 2) == 0)
					reqline[1] = "/index.html";        //Because if no file is specified, index.html will be opened by default (like it happens in APACHE...

				strcpy(path, root);
				strcpy(&path[strlen(root)], reqline[1]);
				printf("file: %s\n", path);

				if ((fd=open(path, O_RDONLY)) != -1){    //FILE FOUND
			
					send(connfd, "HTTP/1.0 200 OK\n\n", 17, 0);
					while ((nbytes=read(fd, buf, BYTES)) > 0)
						write(connfd, buf, nbytes);
				}
				else {
					write(connfd, "HTTP/1.0 404 Not Found\n", 23); //FILE NOT FOUND
				}
			}
		}
	}
	//Closing SOCKET
	shutdown (connfd, SHUT_RDWR);
	close(connfd);
}

char *get_mime_type(char *name) {
	char *extension = strrchr(name, '.');

	if (strcmp(extension, ".html") == 0) {
		return "text/html";
	} else if (strcmp(extension, ".jpg") == 0) {
		return "image/jpeg";
	} else if (strcmp(extension, ".css") == 0) {
		return "text/css";
	} else if (strcmp(extension, ".js") == 0) {
		return "application/javascript";
	}
	return NULL;
}

