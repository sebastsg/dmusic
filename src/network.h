#pragma once

#include "http.h"

struct client_state {
	int socket;
	char* buffer;
	int size;
	int allocated;
	struct http_headers headers;
	bool has_headers;
};

void initialize_network();
void poll_network();
void read_from_client(int socket, char** buffer, int* size, int* allocated, int max_size);
void socket_write_all(int socket, const char* buffer, int size);
void free_network();
