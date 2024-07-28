#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define PLUG_IMPLEMENTATION
#include "plug.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

void* reload_plug(void* num);

void* reload_plug(void* num) {
	char* keyword = "reload";
	char read[32];
	fgets(read, 32, stdin);
	if (strncmp(read, keyword, strlen(keyword)) == 0) {
		void *state = plug_pre_load();
		if(!reload_libplug()) {
			fprintf(stdout, "Failed to reload libplug\n");
			return NULL;
		}
		plug_post_load(state);
	}
}

int main(int argc, char **argv) {

	pthread_t thread;
	pthread_create(&thread, NULL, reload_plug, NULL);

	pthread_join(thread, NULL);

	set_libplug_path("libserver.so");
	
	if(!reload_libplug()) {
		fprintf(stdout, "Failed to load libplug\n");
		return 1;
	}

	plug_init();
	while(1) {
		plug_update();
	}
	plug_free();
	pthread_detach(thread);

	return 0;
}
