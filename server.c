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

#define SIZE 1024
#define HEADER "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n"

void error(char *);
int startServer(char *port);
void respond(int connfd, char *root);
char *get_header(char *type);
char *get_mime_type(char *extension);

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

	listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	bind(listenfd, res->ai_addr, res->ai_addrlen);

	freeaddrinfo(res);

	// listen for incoming connections
	if (listen (listenfd, 5) != 0) {
		perror("listen() error");
		exit(1);
	}
	return listenfd;
}

//client connection
void respond(int connfd, char *root) {
	char mesg[SIZE], buf[SIZE], path[SIZE];
	char *method, *file_path, *protocol;
	int rcvd, fd, nbytes;
	FILE *fp;

	memset(mesg, 0, SIZE);

	rcvd = recv(connfd, mesg, SIZE, 0);

	if (rcvd<0)    // receive error
		fprintf(stderr,("recv() error\n"));
	else if (rcvd==0)    // receive socket closed
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	else {    // message received
		method = strtok(mesg, " \t\n");

		if (strncmp(method, "GET\0", 4) == 0) {
			file_path = strtok(NULL, " \t");

			char *extension = strchr(file_path, '.');

			protocol = strtok(NULL, " \t\n");

			if (strncmp(protocol, "HTTP/1.0", 8) != 0 && strncmp(protocol, "HTTP/1.1", 8) != 0) {
				send(connfd, "HTTP/1.0 400 Bad Request\n", 25, 0);
			} else {

				strcpy(path, root);
				strcpy(&path[strlen(root)], file_path);
				printf("file: %s\n", path);

				if ((fp = fopen(path, "rb"))){    //FILE FOUND
				// if ((fd = open(path, O_RDONLY)) != -1){ 	
					sprintf(buf, HEADER, get_mime_type(extension));
					send(connfd, buf, strlen(buf), 0);

					while ((nbytes = fread(buf, 1, SIZE, fp)) > 0)
						send(connfd, buf, nbytes, 0);
				}
				else {
					send(connfd, "HTTP/1.0 404 Not Found\n", 23, 0); //FILE NOT FOUND
				}
			}
		}
	}
	//Closing SOCKET
	shutdown (connfd, SHUT_RDWR);
	close(connfd);
}

char *get_mime_type(char *extension) {

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
