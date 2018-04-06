/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define ROOT "/Users/Mars/Desktop/CS/project/comp30023-2018-project-1/"
#define HEADER "HTTP/1.0 200 OK\r\nContent-Type: "
#define JS_H "application/javascript\r\n\r\n"
#define CSS_H "text/css\r\n\r\n"
#define JPG_H "image/jpeg\r\n\r\n"
#define HTML_H "text/html\r\n\r\n"

/****************************************************************************/

void process(int client_sock, FILE *FILE, char *path);

/****************************************************************************/

int main(int argc, char **argv) {

	int port;
	char *path;
	int server_sock = -1, client_sock = -1;

	// take the command line argument
	if (argc == 3) {
		port = atoi(argv[1]);
		path = argv[2];
	} else {
		fprintf(stderr, "USAGE: ./server [port number] [path to web root]\n");
		exit(1);
	}
	
	// create a socket
	server_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (server_sock < 0) {
		perror("ERROR on opening socket");
		exit(1);
	} 

	// define the address
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind address to the socket
	if (bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	// listen on socket
	if (listen(server_sock, 5) < 0) {
		perror("ERROR on listening");
		exit(1);
	}

	printf("HTTP server running on port %d\n", port);

	// accept a connection
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	
	while (1) {

		client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &client_len);

		if (client_sock < 0) {
			perror("ERROR on accepting");
			exit(1);
		}

		FILE *file;
		file = fopen(strcat(ROOT, path), "r");

		process(client_sock, file, path);

		fclose(file);
		close(client_sock);
	}

	close(server_sock);

	return 0;
}

void process(int client_sock, FILE *file, char *path) {

	char buf[1024];
	char txt[1024];
	char header[1024] = HEADER;

	// if (strcasecmp(method, "GET")) {
	// 	print_error(file, 501, "Not Implemented");
	// 	exit(1);
	// }

	// if (stat(file, &statbuf) < 0) {
	// 	print_error(file, 404, "Not found");
	// 	exit(1);
	// }

	while (fgets(txt, 1024, file)) {
		strcat(buf, txt);
	}

	char *point = strrchr(path,'.');
    
    if (strcmp(point, ".js") == 0) {
        strcat(header, JS_H);
    } else if (strcmp(point, ".css") == 0) {
      	strcat(header, CSS_H);
    } else if (strcmp(point, ".jpg") == 0) {
      	strcat(header, JPG_H);
    } else if (strcmp(point, ".html") == 0) {
      	strcat(header, HTML_H);
  	}

  	strcat(header, buf);

  	printf("HERE\n");

}

void print_error(FILE *file, int num, char *msg) {

	fprintf(file, "<html><head><title>%d %s</title></head>\r\n", num, msg);
	fprintf(file, "<body><h4>%d %s</h4></body></html>\r\n", num, msg);

}



