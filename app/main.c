#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define PLUG_IMPLEMENTATION
#include "plug.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

#include "server.h"

char *dir = "./";

void* reload_plug(void* arg);
void* app(void* arg);

void* reload_plug(void* arg) {
	size_t loaded = 0;
	while (1) {
		fprintf(stdout, "Type 'reload' to reload the plug\n");
		fprintf(stdout, "Do not press 'Ctrl+D'\n");
		if (arg != NULL) {
			fprintf(stdout, "Invalid argument\n");
			return NULL;
		}
		char* keyword = "reload";
		char read[32];
		fscanf(stdin, "%s", read);
		fprintf(stdout, "Read: %s\n", read);
		if (strncmp(read, keyword, strlen(keyword)) == 0) {
			void *state = plug_pre_load();
			if(!reload_libplug()) {
				fprintf(stdout, "Failed to reload libplug\n");
				return NULL;
			}
			plug_post_load(state);
			loaded++;
			printf("Reloaded %ld times\n", loaded);
		} else if (strncmp(read, "clear", 5) == 0) {
			system("clear");
		} else {
			fprintf(stdout, "Invalid command\n");
			continue;
		}
	}
}

void* app(void* arg) {
	if (arg != NULL) {
		fprintf(stdout, "Invalid argument\n");
		return NULL;
	}
	while(1) {
		plug_update();
	}
}

int main(int argc, char **argv) {

	if (argc == 3)
		dir = argv[2];

#ifdef HOT_RELOADABLE
	if (nob_file_exists("./app")) {
		set_libplug_path("./app/libserver.so");
	} else {
		set_libplug_path("./libserver.so");
	}
#endif

	if(!reload_libplug()) {
		fprintf(stdout, "Failed to load libplug\n");
		return 1;
	}

	plug_init(); {
		Plug *state = plug_pre_load();
		state->dir = dir;
		if(!reload_libplug()) {
			fprintf(stdout, "Failed to load plug\n");
			return 1;
		}
		plug_post_load(state);

		pthread_t app_thread;
		#ifdef HOT_RELOADABLE
		pthread_t inp_thread;
		#endif

		pthread_create(&app_thread, NULL, app, NULL);
		#ifdef HOT_RELOADABLE
		pthread_create(&inp_thread, NULL, reload_plug, NULL);
		#endif

		#ifdef HOT_RELOADABLE
		pthread_join(inp_thread, NULL);
		#endif
		pthread_join(app_thread, NULL);
		#ifdef HOT_RELOADABLE
		pthread_detach(inp_thread);
		#endif
		pthread_detach(app_thread);

	} plug_free();

	return 0;
}
