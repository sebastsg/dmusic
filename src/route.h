#pragma once

#include <stdbool.h>

struct route_result {
	char* body;
	int size;
	char type[64];
	bool freeable;
};

void process_route(struct route_result* result, const char* path, const char* body, int size);
void free_route(struct route_result* result);
