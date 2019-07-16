#pragma once

#include <stdbool.h>
#include <stddef.h>

struct client_state;

struct route_result {
	struct client_state* client;
	char* body;
	size_t size;
	char type[64];
	bool freeable;
};

void process_route(struct route_result* result, const char* path, char* body, size_t size);
void free_route(struct route_result* result);
