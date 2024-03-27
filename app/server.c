#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

struct server {
	int server_fd;
	int client_fd;
};

struct server simpleServer() {
	// Disable output buffering
	setbuf(stdout, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");
	
	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Since the tester restarts your program quite often, setting REUSE_PORT
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEPORT failed: %s \n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	// Set up the server address struct
	struct sockaddr_in serv_addr = { 
		 .sin_family = AF_INET ,
		 .sin_port = htons(4221),
		 .sin_addr = { htonl(INADDR_ANY) },
	};

	// Bind server to port 4221
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	// Listen for incoming connections
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t*) &client_addr_len);	
	printf("Client connected\n");

	return (struct server) { server_fd, client_fd };
}

void serve(int client_fd) {
	char readbuf[1024];
	int bytesread = recv(client_fd, readbuf, sizeof(readbuf), 0);
	if (bytesread < 0) {
		printf("Receive failed: %s \n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Extract the path from the request
	char *reqPath = strtok(readbuf, " ");
	reqPath = strtok(NULL, " ");

	int bytessent;
	char response[512];
	int contentLength;

	if (strcmp(reqPath, "/") == 0) {
		sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 11\r\n\r\nHello World");
	 } else if (strncmp(reqPath, "/echo/", 6) == 0) {
		// parse the content from the request
		reqPath = strtok(reqPath, "/");
		reqPath = strtok(NULL, "");
		contentLength = strlen(reqPath);
		sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", contentLength, reqPath);
		printf("Sending Response: %s\n", response);
	} else if (strcmp(reqPath, "/user-agent") == 0) {
		// parse the user-agent from the request
		reqPath = strtok(NULL, "\r\n");
		reqPath = strtok(NULL, "\r\n");
		reqPath = strtok(NULL, "\r\n");

		// parse the body from the request
		char *body = strtok(reqPath, " "); // body -> user-agent	
		body = strtok(NULL, " "); // body -> curl/x.x.x
		contentLength = strlen(body);								
		sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", contentLength, body);
	} else {
		sprintf(response, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
	}

	bytessent = send(client_fd, response, strlen(response), 0);
	if (bytessent < 0) {
		printf("Send failed: %s \n", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

int main() {
	struct server newServer = simpleServer();
	int server_fd = newServer.server_fd;
	int client_fd = newServer.client_fd;

	if (!fork()) {
		close(server_fd);
		serve(client_fd);
		close(client_fd);
		exit(0);
	}

	return 0;
}
