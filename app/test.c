#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

// Function to compress a string using zlib
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

// Function to print bytes in hexadecimal format
char* hexdump(const unsigned char *data, size_t len) {

	fprintf(stdout, "Data: %s, Data Length: %zu\n", data, len);
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

	printf("Hex String Length: %zu\n", hex_str_len);
	printf("Hex String: \n%s\n", hex_str);
    return hex_str;
}

int main() {
    const char *input = "abc";
    size_t input_len = strlen(input);

    unsigned char *compressed_data = NULL;
    size_t compressed_len = 0;

    // Compress the input string
    int ret = gzip(input, input_len, &compressed_data, &compressed_len);

    if (ret != Z_OK) {
        fprintf(stderr, "Compression failed: %d\n", ret);
        return 1;
    }

    // Print the compressed data in hexadecimal format
	char *hex_str = hexdump(compressed_data, compressed_len);
    // Free allocated memory
    free(compressed_data);
	free(hex_str);

    return 0;
}
