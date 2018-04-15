#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <pthread.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>

struct arg_struct {
    int c;
    char *r;
};

// 1KB size for buffer
#define SIZE 1024
// how many pending connections queue will hold
#define BACKLOG 10
// GET request
#define REQUEST "GET\0"
// specific header for responding
#define HEADER "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n"
// not found header for responding
#define INVALID "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
// html for 404 not found
#define NOT_FOUNT_HTML "<html><body>404 NOT FOUND</body></html>"

int set_server(char *port);
// void respond(int connfd, char *root);
void *respond(void *arguments);
char *get_mime_type(char *extension);
void send_error(FILE *f, char *path);

int main(int argc, char **argv) {

	// take command line arguments: port, root path
	char *port, *root;
	if (argc == 3) {
		port = argv[1];
		root = argv[2];
	} else {
		fprintf(stderr, "usage: ./server [port] [root path]\n");
		exit(1);
	}
	pthread_t newthread;
	int listenfd = set_server(port), connfd = -1;

	printf("HTTP server is listening on port %s\n", port);

	signal(SIGPIPE, SIG_IGN);

	// accept incomping connections
	while (1) {

		connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);

		if (connfd < 0) {
			perror("accept() error");
			exit(1);
		}
		// respond(connfd, root);

		struct arg_struct args;
		args.c = connfd;
		args.r = root;

		if (pthread_create(&newthread , NULL, respond, &args) != 0) {
            perror("pthread_create");
		}
		// pthread_exit(NULL);
	}
	return 0;
}

// client connection
// void respond(int connfd, char *root) {
void *respond(void *arguments) {

	char msg[SIZE], buf[SIZE], path[SIZE];
	char *method, *file_path, *protocol;
	int nbytes, rcvd = -1;
	FILE *f;

	// char *root;
	struct arg_struct *args = (struct arg_struct *)arguments;
	int connfd = args->c;
	// strcpy(root, args->r);

	// make sure empty
	memset(msg, 0, sizeof(msg));

	rcvd = recv(connfd, msg, sizeof(msg), 0);

	if (rcvd < 0){
		// receive error
		fprintf(stderr, "recv() error\n");
	} else if (rcvd == 0){
		// remote side has closed the connection
		fprintf(stderr, "connection closed\n");
	} else {
		// message received
		method = strtok(msg, " \t\n");

		if (strncmp(method, REQUEST, strlen(REQUEST)) == 0) {
			file_path = strtok(NULL, " \t");
			protocol = strtok(NULL, " \t\n");

			// default file: index.html
			if (strncmp(file_path, "/\0", 2)==0) {
				file_path = "/index.html"; 
			}

			char *extension = strrchr(file_path, '.');

			strcpy(path, args->r);
			strcat(path, file_path);
			printf("file: %s\n", path);

			if ((f = fopen(path, "rb"))) {
				// send HTTP header with MIME type
				sprintf(buf, HEADER, get_mime_type(extension));
				send(connfd, buf, strlen(buf), 0);
				// read data and send 1KB at one time
				while ((nbytes = fread(buf, 1, SIZE, f)) > 0) {
					send(connfd, buf, nbytes, 0);
				}
				fclose(f);
			} else {
				// file is not found
				send(connfd, INVALID, strlen(INVALID), 0);
				send(connfd, NOT_FOUNT_HTML, strlen(NOT_FOUNT_HTML), 0);
			}
		}
	}
	// close the socket
	shutdown(connfd, SHUT_RDWR);
	close(connfd);

	pthread_exit(NULL);
	return NULL;
}

//start server
int set_server(char *port) {

	struct addrinfo hints, *res;
	int rv, listenfd = -1;

	// make sure the empty struct
	memset(&hints, 0, sizeof(hints));
	// address family: IPv4 or IPv6
	hints.ai_family = AF_UNSPEC;
	// TCP stream socket type
	hints.ai_socktype = SOCK_STREAM;
	// socket address for bind()
	hints.ai_flags = AI_PASSIVE;
	
	// get ready to connect
	if ((rv = getaddrinfo(NULL, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}
	// create a socket
	listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	// bind socket to the port in getaddrinfo()
	bind(listenfd, res->ai_addr, res->ai_addrlen);
	// free the linked list
	freeaddrinfo(res);
	// listen for incoming connections
	if (listen(listenfd, BACKLOG) != 0) {
		perror("listen() error");
		exit(1);
	}
	return listenfd;
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
