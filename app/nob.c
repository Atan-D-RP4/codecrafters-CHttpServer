#include <stdio.h>
#include <stdlib.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

#define COMPILER "clang"

bool hot_reload = true;

Nob_String_View CFLAGS_ARR[] = {
	(Nob_String_View) { .data = "-Wall", .count = 5 },
	(Nob_String_View) { .data = "-Wextra", .count = 7 },
	(Nob_String_View) { .data = "-pedantic", .count = 9 },
	(Nob_String_View) { .data = "-O3", .count = 3 },
	(Nob_String_View) { .data = "-ggdb", .count = 5 },
};

Nob_String_View LDFLAGS_ARR[] = {
	(Nob_String_View) { .data = "-lz", .count = 3},
	(Nob_String_View) { .data = "-lpthread", .count =  9 },
};

Nob_String_View PLUGFLAGS_ARR[] = {
	(Nob_String_View) { .data = "-fPIC", .count = 5 },
	(Nob_String_View) { .data = "-shared", .count = 8 },
};

bool build_main();
bool build_server();

char* outfile = NULL;

int main(int argc, char **argv) {
	NOB_GO_REBUILD_URSELF(argc, argv);

	char* app_args = nob_shift_args(&argc, &argv);
	char* subcmd = NULL;

	if (argc <= 0) {
		subcmd = "build";
	} else {
		subcmd = nob_shift_args(&argc, &argv);
	}
	fprintf(stdout, "Subcommand: %s\n", subcmd);

	if (argc > 0) {
		outfile = nob_shift_args(&argc, &argv);
		fprintf(stdout, "Output file: %s\n", outfile);
	}

	if (strcmp(subcmd, "build") == 0) {
		if (!build_main()) {
			fprintf(stderr, "Failed to build main\n");
			return 1;
		}

		if (!build_server()) {
			fprintf(stderr, "Failed to build server\n");
			return 1;
		}
	} else if (strcmp(subcmd, "reload") == 0) {
		if (!build_server()) {
			fprintf(stderr, "Failed to build server\n");
			return 1;
		}
	} else if (strcmp(subcmd, "run") == 0) {
		Nob_Cmd cmd = { 0 };
		nob_cmd_append(&cmd, "./main");
		if (!nob_cmd_run_async(cmd)) return 1;
	} else if (strcmp(subcmd, "clean") == 0) {
		Nob_Cmd cmd = { 0 };
		nob_cmd_append(&cmd, "rm", "main", "libserver.so", "nob", "nob.old" );
		if (!nob_cmd_run_async(cmd)) return 1;
	} else {
		fprintf(stderr, "Unknown subcommand: %s\n", subcmd);
		return 1;
	}
	return 0;
}

bool build_main() {
	// Check if Current working directory is the same as the source directory
	Nob_Cmd cmd = { 0 };
	nob_cmd_append(&cmd, COMPILER);

	if (outfile == NULL) {
		if (nob_file_exists("./app")) {
			outfile = "./app/main";
		} else {
			outfile = "./main";
		}
	}
	nob_cmd_append(&cmd, "-o", outfile);

	if (nob_file_exists("main.c")) {
		nob_cmd_append(&cmd, "./main.c");
	} else {
		nob_cmd_append(&cmd, "./app/main.c");
	}

	if (hot_reload) {
		nob_cmd_append(&cmd, "-DHOT_RELOADABLE");
	} else {
		if (nob_file_exists("app")) {
			nob_cmd_append(&cmd, "./app/libserver.so");
		} else {
			nob_cmd_append(&cmd, "./libserver.so");
		}
	}

	for (int i = 0; i < sizeof(CFLAGS_ARR) / sizeof(CFLAGS_ARR[0]); i++) {
		nob_cmd_append(&cmd, CFLAGS_ARR[i].data);
	}

	if (!nob_cmd_run_async(cmd)) return false;

	return true;
}

bool build_server() {
	Nob_Cmd cmd = { 0 };
	nob_cmd_append(&cmd, COMPILER);

	for (int i = 0; i < sizeof(PLUGFLAGS_ARR) / sizeof(PLUGFLAGS_ARR[0]); i++) {
		nob_cmd_append(&cmd, PLUGFLAGS_ARR[i].data);
	}

	if (nob_file_exists("./app")) {
		nob_cmd_append(&cmd, "-o", "./app/libserver.so");
		nob_cmd_append(&cmd, "./app/server.c", "./app/plug.c");
	} else {
		nob_cmd_append(&cmd, "server.c", "plug.c");
		nob_cmd_append(&cmd, "-o", "libserver.so");
	}

	for (int i = 0; i < sizeof(CFLAGS_ARR) / sizeof(CFLAGS_ARR[0]); i++) {
		nob_cmd_append(&cmd, CFLAGS_ARR[i].data);
	}

	for (int i = 0; i < sizeof(LDFLAGS_ARR) / sizeof(LDFLAGS_ARR[0]); i++) {
		nob_cmd_append(&cmd, LDFLAGS_ARR[i].data);
	}

	if (!nob_cmd_run_async(cmd)) return false;

	return true;
}
