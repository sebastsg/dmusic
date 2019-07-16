#pragma once

#define _GNU_SOURCE

#include <stdbool.h>
#include <stddef.h>

struct client_state;

struct http_headers {
	size_t content_length;
	char method[16];
	char connection[16];
	char status[32];
	char resource[128];
	char content_type[256];
};

struct http_data_part {
	char name[32];
	const char* value;
	const char* filename;
	size_t size;
	int index;
};

struct http_data {
	struct http_data_part* parameters;
	int count;
	int allocated;
};

int http_data_parameter_array_size(struct http_data* data, const char* name);
struct http_data http_load_data(char* buffer, size_t size, const char* content_type);
void http_free_data(struct http_data* data);
struct http_data_part* http_data_param(struct http_data* data, const char* name, int index);
const char* http_data_string(struct http_data* data, const char* name);
const char* http_data_string_at(struct http_data* data, const char* name, int index);

bool http_read_headers(struct client_state* client);
bool http_read_body(struct client_state* client);
void http_write_headers(struct client_state* client, const struct http_headers* headers);
const char* http_file_content_type(const char* path);
