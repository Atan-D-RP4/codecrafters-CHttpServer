#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define PLUG_IMPLEMENTATION
#include "plug.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

#include "server.c"

char *dir = "./";
void* reload_plug(void* arg);

typedef struct {
	int server_fd;
	char *dir;
} Plug;

void* reload_plug(void* arg) {
	size_t loaded = 0;
	while (1) {
		if (arg != NULL) {
			fprintf(stdout, "Invalid argument\n");
			return NULL;
		}
		char* keyword = "reload";
		char read[32];
		fscanf(stdin, "%s", read);
		if (strncmp(read, keyword, strlen(keyword)) == 0) {
			void *state = plug_pre_load();
			if(!reload_libplug()) {
				fprintf(stdout, "Failed to reload libplug\n");
				return NULL;
			}
			plug_post_load(state);
		}
		loaded++;
		printf("Reloaded %ld times\n", loaded);
	}
}

void* app(void* arg) {
	if (arg != NULL) {
		fprintf(stdout, "Invalid argument\n");
		return NULL;
	}
	set_libplug_path("./libserver.so");

	if(!reload_libplug()) {
		fprintf(stdout, "Failed to load libplug\n");
		return NULL;
	}

	plug_init();
	Plug *state = plug_pre_load();
	state->dir = dir;
	if(!reload_libplug()) {
		fprintf(stdout, "Failed to load plug\n");
		return NULL;
	}
	plug_post_load(state);
	while(1) {
		plug_update();
	}
	plug_free();
}

int main(int argc, char **argv) {

	if (argc == 3)
		dir = argv[2];

	pthread_t inp_thread, app_thread;
	pthread_create(&inp_thread, NULL, reload_plug, NULL);
	pthread_create(&app_thread, NULL, app, NULL);

	pthread_join(inp_thread, NULL);
	pthread_join(app_thread, NULL);

	pthread_detach(inp_thread);
	pthread_detach(app_thread);

	return 0;
}
