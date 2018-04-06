/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

/****************************************************************************/

void process(int client_sock);

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

		process(client_sock);

		close(client_sock);
	}

	close(server_sock);

	return 0;
}

void process(int client_sock) {

	char buf[1024];
	char path[512];
	char method[255];


}



