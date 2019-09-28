#include "route.h"
#include "files.h"
#include "http.h"
#include "system.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void route_file(struct route_result* result, const char* path) {
	print_info_f("Serving file: %s", path);
	size_t file_size = 0;
	result->body = read_file(path, &file_size);
	result->size = file_size;
	result->freeable = true;
	strcpy(result->type, http_file_content_type(path));
}
