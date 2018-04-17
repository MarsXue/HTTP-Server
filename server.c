/* * * * * * * * *
 * A basic multi-thread HTTP server - responds GET requests only
 * 
 * Command line arguments: port number, string path to root web directory
 * Server supports responding with 200 (success) and 400 (not found)
 * Available file extentsion: HTML, JPEG, CSS, JavaScript
 *
 * created for COMP30023 Computer Systems - Assignment 1, 2018
 * by Wenqing Xue <wenqingx@student.unimelb.edu.au>
 * Login ID: wenqingx
 */

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<netdb.h>
#include<signal.h>
// #include<fcntl.h>

// helper structure to store arguments for pthread_create()
struct args_struct {
	// accepted connection
    int new_connfd;
    // string path to root web directory
    char *root_path;
};

// 4KB size for buffer
#define SIZE 4096
// how many pending connections queue will hold
#define BACKLOG 10
// GET request
#define REQUEST "GET\0"
// specific header for responding
#define HEADER "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n"
// not found header for responding
#define INVALID "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
// html content for 404 not found
#define NOT_FOUNT_HTML "<html><body>404 NOT FOUND</body></html>"

/******************************* HELP FUNCTION *******************************/
int set_server(char *port);
void *respond(void *arguments);
char *get_mime_type(char *extension);
/****************************************************************************/

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

	// make sure the server is running
	printf("HTTP server is listening on port %s\n", port);

	signal(SIGPIPE, SIG_IGN);

	// accept incomping connections
	while (1) {

		connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);

		// accept() returns -1 if an error occurs
		if (connfd < 0) {
			perror("accept() error");
			exit(1);
		}

		// pass the connfd and root into struct
		struct args_struct args;
		args.new_connfd = connfd;
		args.root_path = root;

		// multi-thread to process incoming requests and sending responses
		if (pthread_create(&newthread , NULL, respond, &args) != 0) {
            perror("pthread_create() error");
		}
		// here???
		pthread_detach(newthread);
	}
	return 0;
}

// client connection
void *respond(void *arguments) {

	char msg[SIZE], buf[SIZE], path[SIZE];
	char *method, *file_path;
	// char *protocol;
	int nbytes, rcvd = -1;
	FILE *file;

	struct args_struct *args = (struct args_struct *)arguments;
	int connfd = args->new_connfd;

	// make sure empty
	bzero(msg, sizeof(msg));
	bzero(buf, sizeof(buf));
	bzero(path, sizeof(path));

	rcvd = recv(connfd, msg, sizeof(msg), 0);

	if (rcvd < 0){
		// receive error
		fprintf(stderr, "recv() error\n");
	} else if (rcvd == 0){
		// remote side has closed the connection
		fprintf(stderr, "recv = 0\n");
	} else {
		// message received
		method = strtok(msg, " \t\n");

		// only process with GET request
		if (strncmp(method, REQUEST, strlen(REQUEST)) == 0) {
			file_path = strtok(NULL, " \t");
			// protocol = strtok(NULL, " \t\n");

			// default file: index.html
			if (strncmp(file_path, "/\0", 2) == 0) {
				file_path = "/index.html"; 
			}

			// get the extension by checking the last dot
			char *extension = strrchr(file_path, '.');

			// path = root_path + file_path
			strcpy(path, args->root_path);
			strcat(path, file_path);
			printf("file: %s\n", path);

			// 200 response containing the requested file
			if ((file = fopen(path, "rb"))) {
				// send HTTP header with MIME type
				sprintf(buf, HEADER, get_mime_type(extension));
				send(connfd, buf, strlen(buf), 0);
				// read data and send 4KB at one time
				while ((nbytes = fread(buf, sizeof(char), SIZE, file)) > 0) {
					send(connfd, buf, nbytes, 0);
				}
				fclose(file);
			} 
			// 404 response if the requested file is not found
			else {
				// send HTTP header for 404 status
				send(connfd, INVALID, strlen(INVALID), 0);
				// send string of html content for 404 not found
				send(connfd, NOT_FOUNT_HTML, strlen(NOT_FOUNT_HTML), 0);
			}
		}
	}
	// close the socket
	shutdown(connfd, SHUT_RDWR);
	close(connfd);

	// exit the multi-thread
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

	// create a socket, returns -1 if error
	listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (listenfd < 0) {
		perror("socket() error");
		exit(1);
	}

	// bind socket to the port in getaddrinfo(), returns -1 if error
	if (bind(listenfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind() error");
		exit(1);
	}

	// free the linked list
	freeaddrinfo(res);

	// listen for incoming connections, returns -1 if error
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
