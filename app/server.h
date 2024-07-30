#ifndef SERVER_H_
#define SERVER_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	int server_fd;
	char *dir;
} Plug;

typedef struct {
	char method[16];
	char path[256];
	char reqtype[16];

	char host[128];
	char userAgent[128];
	char accept[8];
	char encoding[256];
	char useEncoding[16];
} Header;

typedef enum {
	INDEX,
	ECHO,
	USER_AGENT,
	GET_FILE,
	POST_FILE,
	REDIRECT,
	ERROR,
	NOT_FOUND
} RESPONSE_TYPE;

char* hexdump(char* data, size_t len);
int gzip(const char *input, size_t input_len, unsigned char **output, size_t *output_len);
int simpleServer();
void serve(int client_fd);
void server_loop(int server_fd);
Header* parseHeaders(char* readbuf, int bytesread);
RESPONSE_TYPE getResponseType(char* path, char* method);

#endif // SERVER_H_
