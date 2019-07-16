#pragma once

#include "http.h"

struct client_state {
	int socket;
	char* buffer;
	size_t size;
	size_t allocated;
	struct http_headers headers;
	bool has_headers;
};

void initialize_network();
void poll_network();
void read_from_client(int socket, char** buffer, size_t* size, size_t* allocated, size_t max_size);
void socket_write_all(int socket, const char* buffer, size_t size);
void free_network();
