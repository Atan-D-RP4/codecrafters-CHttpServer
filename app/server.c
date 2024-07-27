#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <zlib.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

char *dir = "./";

char* hexdump(char* data, size_t len) {

    size_t hex_str_len = len * 4;
    char *hex_str = malloc(hex_str_len);

    if (hex_str == NULL) {
        return NULL;
    }

    size_t pos = 0;
    for (size_t i = 0; i < len; ++i) {
        pos += snprintf(hex_str + pos, hex_str_len - pos, "%02X ", data[i]);
        if ((i + 1) % 8 == 0 && i + 1 < len) {
            snprintf(hex_str + pos, hex_str_len - pos, "\n");
            pos++;
        }
    }

	fprintf(stdout, "Hexdump Len: %zu\n", strlen(hex_str));
	fprintf(stdout, "Hexdump:\n%s\n", hex_str);

    return hex_str;
}

int gzip(const char *input, size_t input_len, unsigned char **output, size_t *output_len) {
    // Estimate the maximum compressed length and allocate memory
    size_t max_compressed_len = compressBound(input_len);
    *output = malloc(max_compressed_len);

	z_stream zs = {
		.zalloc = Z_NULL,
		.zfree = Z_NULL,
		.opaque = Z_NULL,
		.avail_in = input_len,
		.next_in = (unsigned char *)input,
		.avail_out = max_compressed_len,
		.next_out = *output,
	};


	// Initialize the compression stream
	int ret = deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);

	if (ret != Z_OK) {
		return ret;
	}

	printf("Input Data: %s\n", input);
	// Perform the compression
	ret = deflate(&zs, Z_FINISH);

	while (ret == Z_OK) {
		// If the output buffer is full, reallocate memory
		if (zs.avail_out == 0) {
			*output = realloc(*output, max_compressed_len * 2);
			zs.avail_out = max_compressed_len;
			zs.next_out = *output + zs.total_out;
			max_compressed_len *= 2;
		}

		// Continue compressing
		ret = deflate(&zs, Z_FINISH);
	}

	// Finalize the compression
	ret = deflateEnd(&zs);

	if (ret != Z_OK) {
		return ret;
	}

	// Set the output length
	*output_len = zs.total_out;

	return Z_OK;
}

// This function sets up a simple server that listens on port 4221
int simpleServer() {
	// Disable output buffering
	setbuf(stdout, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	int server_fd;

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

	return server_fd;
}

void serve(int client_fd) {
	char readbuf[2048];
	int bytesread = recv(client_fd, readbuf, sizeof(readbuf), 0);
	if (bytesread < 0) {
		printf("Receive failed: %s \n", strerror(errno));
		return;
	}

	fprintf(stdout, "Received:\n%s\n", readbuf);

	char method[16], reqtype[16], path[256];
	sscanf(readbuf, "%s %s %[^\r]", method, path, reqtype);

	// Headers
	char host[128], userAgent[128], accept[8];
	char *headers = strtok(readbuf, "\r\n");
	headers = strtok(NULL, "\r\n");

	if (headers != NULL && strncmp(headers, "Host: ", 6) == 0) {
		sscanf(headers, "Host: %s", host);
		headers = strtok(NULL, "\r\n");
	} else {
		strcpy(host, "localhost");
	}

	if (headers != NULL && strncmp(headers, "User-Agent: ", 12) == 0) {
		sscanf(headers, "User-Agent: %s", userAgent);
		headers = strtok(NULL, "\r\n");
	} else {
		strcpy(userAgent, "curl/7.68.0");
	}

	if (headers != NULL && strncmp(headers, "Accept: ", 8) == 0) {
		sscanf(headers, "Accept: %s", accept);
		headers = strtok(NULL, "\r\n");
	} else {
		strcpy(accept, "*/*");
	}

	char encoding[128];
	if (headers != NULL && strncmp(headers, "Accept-Encoding: ", 17) == 0) {
		sscanf(headers, "Accept-Encoding: %[^\r]", encoding);
	} else {
		strcpy(encoding, "identity");
	}

	printf("Parsed Headers:\n");
	printf("Method: %s\n", method);
	printf("Path: %s\n", path);
	printf("Request Type: %s\n", reqtype);
	printf("Host: %s\n", host);
	printf("User-Agent: %s\n", userAgent);
	printf("Accept: %s\n", accept);
	printf("Accept-Encoding: %s\n", encoding);

	char validEncodings[3][8] = {"gzip", "deflate", "identity"};
	char useEncoding[16] = { 0 };
	char* en_tok = strtok(encoding, ",");
	while (en_tok != NULL) {
		// Remove leading whitespace
		while (*en_tok == ' ') {
			en_tok++;
		}
		// Remove trailing whitespace
		char *end = en_tok + strlen(en_tok) - 1;
		while (end > en_tok && *end == ' ') {
			*end-- = '\0';
		}

		printf("Encoding Token: %s\n", en_tok);
		for (int i = 0; i < 3; i++) {
			if (strcmp(en_tok, validEncodings[i]) == 0) {
				strcpy(useEncoding, en_tok);
				break;
			}
		}
		en_tok = strtok(NULL, ",");
	}

	printf("\n");

	// Extract the path from the request
	char *reqPath = path;

	int bytessent;
	char response[512];
	int contentLength;

	if (strcmp(reqPath, "/") == 0) {
		sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 11\r\n\r\nHello World");
	} else if (strncmp(reqPath, "/echo/", 6) == 0) {

		// parse the content from the request
		reqPath = strtok(reqPath, "/");
		reqPath = strtok(NULL, "");
		char *content = reqPath;
		contentLength = strlen(content);

		if (strlen(useEncoding) > 0) {
			if (strcmp(useEncoding, "gzip") == 0) {
				char *compressed = NULL;
				size_t compressedLength = 0;
				if (gzip(content, contentLength, (unsigned char **)&compressed, &compressedLength) != Z_OK) {
					printf("Failed to compress content\n");
					strcpy(useEncoding, "identity");
				} else {
					content = compressed;
					contentLength = compressedLength;
					fprintf(stdout, "Content Length: %d\n", contentLength);
					fprintf(stdout, "Content: %s\n", content);
				}
			}
			sprintf(response, "HTTP/1.1 200 OK\r\nContent-Encoding: %s\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s",
					useEncoding, contentLength, content);
		} else {
			sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s",
					contentLength, content);
		}

	} else if (strcmp(reqPath, "/user-agent") == 0) {

		// parse the user-agent from the request
		//	char *userAgent = strtok(readbuf, "\r\n");
		//	userAgent = strtok(NULL, "\r\n");
		//	userAgent = strtok(NULL, " ");
		//	userAgent = strtok(NULL, " ");
		//	userAgent = strtok(userAgent, "\r\n");
		printf("User-Agent: %s\n", userAgent);

		// parse the body from the request
		contentLength = strlen(userAgent);
		sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", contentLength, userAgent);
	} else if (strncmp(reqPath, "/files/", 7) == 0 && strcmp(method, "GET") == 0) {

		// parse the file path
		reqPath = strtok(reqPath, "/");
		printf("ReqPath: %s\n", reqPath);
		reqPath = strtok(NULL, "/");
		printf("ReqPath: %s\n", reqPath);

		char filename[256];
		sprintf(filename, "%s%s", dir, reqPath);
		printf("Filename: %s\n", filename);

		FILE *fp = fopen(filename, "rb");
		if (!fp) {
			printf("File not found: %s\n", filename);
			sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
			bytessent = send(client_fd, response, strlen(response), 0);
		} else if (strcmp(reqPath, "/redirect") == 0) {
			sprintf(response, "HTTP/1.1 301 Moved Permanently\r\nLocation: http://www.google.com\r\n\r\n");
		} else if (strcmp(reqPath, "/error") == 0) {
			sprintf(response, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
		}  else {
			printf("Opening file %s\n", filename);
		}

		if (fseek(fp, 0, SEEK_END) < 0) {
			printf("Seek failed: %s\n", strerror(errno));
			sprintf(response, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
		}

		// Get the size of the file
		size_t data_size = ftell(fp);

		// Reset the file pointer to the beginning of the file
		rewind(fp);

		// Allocate memory to store the file
		void *data = malloc(data_size);

		// Fill the data buffer
		if (fread(data, 1, data_size, fp) != data_size) {
			printf("Read failed: %s\n", strerror(errno));
			sprintf(response, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
		}

		// Close the file
		fclose(fp);

		// Send the response
		sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %ld\r\n\r\n%s", data_size, (char *) data);
	} else if (strncmp(reqPath, "/files/", 7) == 0 && strcmp(method, "POST") == 0) {

		// parse the file path
		reqPath = strtok(reqPath, "/");
		printf("ReqPath: %s\n", reqPath);
		reqPath = strtok(NULL, "/");
		printf("ReqPath: %s\n", reqPath);

		char filename[256];
		sprintf(filename, "%s%s", dir, reqPath);
		printf("Filename: %s\n", filename);

		FILE *fp = fopen(filename, "wb");
		if (!fp) {
			printf("Failed to open file: %s\n", filename);
			sprintf(response, "HTTP/1.1 404 NOT FOUND\r\n\r\n");
			bytessent = send(client_fd, response, strlen(response), 0);
		} else if (strcmp(reqPath, "/redirect") == 0) {
			sprintf(response, "HTTP/1.1 301 Moved Permanently\r\nLocation: http://www.google.com\r\n\r\n");
		} else if (strcmp(reqPath, "/error") == 0) {
			sprintf(response, "HTTP/1.1 500 INTERNAL SERVER ERROR\r\n\r\n");
		} else {
			printf("Creating file %s\n", filename);
		}

		printf("Received: %s\n\n", readbuf);
		// char *parser = strtok(readbuf, "\r\n");


		Nob_String_View buf = {
			.data = readbuf,
			.count = bytesread
		};
		nob_log(NOB_INFO, "Size of Message: %zu\n", buf.count);

		// The tokenizer
		Nob_String_View content = {
			.data = buf.data,
			.count = buf.count
		};
		Nob_String_View token  = nob_sv_chop_by_delim(&content, '\n');
		for (size_t i = 0; i < 100 && content.count > 0; ++i) {
			content = nob_sv_trim_left(content);
			token  = nob_sv_chop_by_delim(&content, '\n');
			nob_log(NOB_INFO, "  "SV_Fmt, SV_Arg(token));
		}


		printf("Content Length: %lu\n", token.count);
		printf("Content to Write: %s\n", token.data);

		fwrite(token.data, sizeof(char), token.count, fp);
		fclose(fp);

		sprintf(response, "HTTP/1.1 201 Created\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n");

	} else if (strcmp(reqPath, "/redirect") == 0) {
		sprintf(response, "HTTP/1.1 301 Moved Permanently\r\nLocation: http://www.google.com\r\n\r\n");
	} else if (strcmp(reqPath, "/error") == 0) {
		sprintf(response, "HTTP/1.1 500 INTERNAL SERVER ERROR\r\n\r\n");
	} else {
		sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
	}

	printf("Sending Response: \n%s\n", response);
	bytessent = send(client_fd, response, strlen(response), 0);
	if (bytessent < 0) {
		printf("Send failed: %s \n", strerror(errno));
		return;
	}
}

int main(int argc, char *argv[]) {
	if (argc == 3)
		dir = argv[2];

	int server_fd = simpleServer();

	int client_addr_len;
	struct sockaddr_in client_addr;

	while(1) {
		printf("Waiting for a client to connect...\n");
		client_addr_len = sizeof(client_addr);
		int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t*) &client_addr_len);
		if (client_fd < 0) {
			printf("Accept failed: %s \n", strerror(errno));
			continue;
		}
		printf("Client connected\n");

		pid_t pid = fork();
		if (pid == -1) {
			printf("Fork failed: %s\n", strerror(errno));
			close(client_fd);
			continue;
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
	close(server_fd);

	return 0;
}
