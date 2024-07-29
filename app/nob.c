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
	(Nob_String_View) { .data = "-I./", .count = 12 },
	(Nob_String_View) { .data = "-Wl,-rpath=./", .count = 21 },
	(Nob_String_View) { .data = "-lz", .count = 3},
	(Nob_String_View) { .data = "-lpthread", .count =  9 },
};

Nob_String_View PLUGFLAGS_ARR[] = {
	(Nob_String_View) { .data = "-DSERVER_IMPLEMENTATION", .count = 23 },
	(Nob_String_View) { .data = "-fPIC", .count = 5 },
	(Nob_String_View) { .data = "-shared", .count = 8 },
};

int main(int argc, char **argv) {
	NOB_GO_REBUILD_URSELF(argc, argv);

	char* app_args = nob_shift_args(&argc, &argv);
	Nob_Cmd cmd = { 0 };
	nob_cmd_append(&cmd, COMPILER);

	nob_cmd_append(&cmd, "-o", "libserver.so");
	nob_cmd_append(&cmd, "server.c", "plug.c");

	for (int i = 0; i < sizeof(CFLAGS_ARR) / sizeof(CFLAGS_ARR[0]); i++) {
		nob_cmd_append(&cmd, CFLAGS_ARR[i].data);
	}

	for (int i = 0; i < sizeof(LDFLAGS_ARR) / sizeof(LDFLAGS_ARR[0]); i++) {
		nob_cmd_append(&cmd, LDFLAGS_ARR[i].data);
	}

	for (int i = 0; i < sizeof(PLUGFLAGS_ARR) / sizeof(PLUGFLAGS_ARR[0]); i++) {
		nob_cmd_append(&cmd, PLUGFLAGS_ARR[i].data);
	}

	if (!nob_cmd_run_sync(cmd)) return 1;
	cmd.count = 0;

	nob_cmd_append(&cmd, COMPILER);
	nob_cmd_append(&cmd, "-o", "main");
	nob_cmd_append(&cmd, "main.c");

	if (hot_reload) {
		nob_cmd_append(&cmd, "-DHOT_RELOADABLE");
	} else {
		nob_cmd_append(&cmd, "libserver.so");
	}

	for (int i = 0; i < sizeof(CFLAGS_ARR) / sizeof(CFLAGS_ARR[0]); i++) {
		nob_cmd_append(&cmd, CFLAGS_ARR[i].data);
	}

	if (!nob_cmd_run_async(cmd)) return 1;

	return 0;
}
