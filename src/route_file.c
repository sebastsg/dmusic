#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "files.h"
#include "http.h"
#include "network.h"
#include "route.h"
#include "system.h"

void route_file(struct route_result* result, const char* path) {
	size_t file_size = 0;
	char* range = result->client->headers.range;
	if (range) {
		const char* dash = strchr(range, '-');
		if (dash) {
			const size_t dash_index = dash - range;
			if (dash_index > 6) {
				size_t buffer_size = 0;
				range[dash_index] = '\0';
				const size_t begin = atoi(range + 6);
				const size_t end = range[dash_index + 1] != '\0' ? (size_t)atoi(range + dash_index + 1) : 0xffffffff;
				print_info_f("Serving file range (%zu - %zu): %s", begin, end, path);
				result->body = read_file_section(path, &file_size, &buffer_size, begin, end);
				result->size = buffer_size;
				strcpy(result->client->headers.status, "206");
				sprintf(result->client->headers.content_range, "bytes %zu-%zu/%zu", begin, begin + buffer_size - 1, file_size);
			}
		}
	}
	if (!result->body) {
		print_info_f("Serving file: %s", path);
		result->body = read_file(path, &file_size);
		result->size = file_size;
	}
	result->freeable = true;
	strcpy(result->type, http_file_content_type(path));
}
