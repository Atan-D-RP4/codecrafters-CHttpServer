#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plug.h"

#ifndef SERVER_IMPLEMENTATION
	#define SERVER_IMPLEMENTATION
#else
	#undef SERVER_IMPLEMENTATION
#endif

#include "server.c"

typedef struct {
	int server_fd;
	char *dir;
} Plug;

static Plug *plug = NULL;

void plug_update() {
	server_loop(plug->server_fd);
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

	memcpy(plug, cpy, sizeof(Plug));
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
