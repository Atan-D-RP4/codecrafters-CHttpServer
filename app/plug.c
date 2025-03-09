#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "plug.h"

#ifndef SERVER_IMPLEMENTATION
	#define SERVER_IMPLEMENTATION
#else
	#undef SERVER_IMPLEMENTATION
#endif

#include "server.h"

Plug *plug = NULL;

void plug_update() {
	int server_fd = plug->server_fd;
	int client_addr_len;
	struct sockaddr_in client_addr;
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t*) &client_addr_len);
	if (client_fd < 0) {
		printf("Accept failed: %s \n", strerror(errno));
		return;
	}
	printf("Client connected\n");

	pid_t pid = fork();
	if (pid == -1) {
		printf("Fork failed: %s\n", strerror(errno));
		close(client_fd);
		return;
	} else if (pid == 0) {
		// Child process
		close(server_fd);
		serve(client_fd);
		exit(0);
	} else {
		// Parent process
		close(client_fd);
	}
}

void plug_init() {
	fprintf(stderr, "Initializing Global State\n");
	plug = malloc(sizeof(Plug));

	plug->server_fd = simpleServer();
	plug->dir = "./";
	fprintf(stdout, "Server started on port 4221\n");
}

void plug_post_load(void *state) {
	fprintf(stderr, "Plug Loaded\n");
	Plug *cpy = (Plug *) state;
	plug_init();
	plug->dir = cpy->dir;
	fprintf(stderr, "Saved Dir: %s\n", plug->dir);

	close(cpy->server_fd);
	free(cpy);
	fprintf(stderr, "Reloaded Global State\n");
}

void* plug_pre_load() {
	fprintf(stderr, "Preparing to Reload Plug\n");
	return (void *) plug;
}

void plug_free() {
	fprintf(stderr, "Freeing Global State\n");
	free(plug);
}
