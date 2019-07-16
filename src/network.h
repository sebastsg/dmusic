#pragma once

#include "http.h"

#include <time.h>

struct client_state {
	int socket;
	char* buffer;
	size_t size;
	size_t allocated;
	struct http_headers headers;
	bool has_headers;
	time_t last_request;
};

void initialize_network();
void poll_network();
void read_from_client(int socket, char** buffer, size_t* size, size_t* allocated, size_t max_size);
void socket_write_all(struct client_state* client, const char* buffer, size_t size);
void free_network();
