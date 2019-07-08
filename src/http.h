#pragma once

#include <stdbool.h>

struct client_state;

struct http_headers {
	int content_length;
	char method[16];
	char connection[16];
	char status[32];
	char resource[128];
	char content_type[256];
};

bool http_read_headers(struct client_state* client);
bool http_read_body(struct client_state* client);
void http_write_headers(int socket, const struct http_headers* headers);
const char* http_file_content_type(const char* path);
