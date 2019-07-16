#include "http.h"
#include "network.h"
#include "format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void http_read_content_length(struct http_headers* dest, const char* src) {
	const char* begin = strstr(src, "Content-Length: ");
	if (begin) {
		begin += strlen("Content-Length: ");
		const char* end = strstr(begin, "\r\n");
		if (end) {
			char buffer[32];
			string_copy_substring(buffer, begin, end - begin);
			sscanf(buffer, "%zu", &dest->content_length);
		}
	}
}

static void http_read_method(char* dest, const char* src) {
	const char* end = strchr(src, ' ');
	if (end) {
		string_copy_substring(dest, src, end - src);
	}
}

static void http_read_resource(char* dest, const char* src) {
	const char* begin = strchr(src, ' ');
	if (begin) {
		begin += 2;
		const char* end = strchr(begin, ' ');
		if (end) {
			string_copy_substring(dest, begin, end - begin);
			url_decode(dest);
		}
	}
}

static void http_read_content_type(char* dest, const char* src) {
	const char* begin = strstr(src, "Content-Type: ");
	if (begin) {
		begin += strlen("Content-Type: ");
		const char* end = strstr(begin, "\r\n");
		if (end) {
			string_copy_substring(dest, begin, end - begin);
		}
	}
}

static char* http_next_form_data(char* buffer, size_t buffer_size, const char* boundary, const char** name, size_t* size, const char** filename) {
	buffer++; // buffer could be \0 (see end). maybe this is bad design, but should work fine
	buffer_size--;
	char* begin = strstr(buffer, boundary); // don't care about "--" prefix, at least for now
	if (!begin) {
		return NULL;
	}
	size_t boundary_size = strlen(boundary);
	begin += boundary_size + 2; // also skip "\r\n"
	size_t begin_size = buffer_size - (begin - buffer);
	char* end = memmem(begin, begin_size, boundary, boundary_size);
	if (!end) {
		return NULL;
	}
	end -= 4; // skip "\r\n--"
	char* content = strstr(begin, "\r\n\r\n");
	if (!content || content >= end) {
		return NULL;
	}
	content += 4; // skip "\r\n\r\n"
	if (size) {
		*size = end - content;
	}
	*end = '\0';
	char* begin_disposition = strstr(begin, "Content-Disposition: ");
	if (!begin_disposition) {
		return content;
	}
	char* disposition_name = strstr(begin_disposition, "name=\"");
	char* disposition_filename = strstr(begin_disposition, "filename=\"");
	if (name && disposition_name) {
		disposition_name += strlen("name=\"");
		char* end_disposition_name = strchr(disposition_name, '"');
		if (end_disposition_name) {
			*name = disposition_name;
			*end_disposition_name = '\0';
		}
	}
	if (filename) {
		if (disposition_filename) {
			disposition_filename += strlen("filename=\"");
			char* end_disposition_filename = strchr(disposition_filename, '"');
			if (end_disposition_filename) {
				*filename = disposition_filename;
				*end_disposition_filename = '\0';
			}
		} else {
			*filename = NULL;
		}
	}
	return content;
}

static void http_get_boundary(const char* content_type, char* dest) {
	const char* begin = strstr(content_type, "boundary=");
	if (begin) {
		begin += strlen("boundary=");
		const char* end = begin + strlen(begin);
		if (end) {
			size_t length = end - begin;
			string_copy_substring(dest, begin, length > 70 ? 70 : length);
		}
	}
}

int http_data_parameter_array_size(struct http_data* data, const char* name) {
	for (int i = data->count - 1; i >= 0; i--) {
		if (!strcmp(data->parameters[i].name, name)) {
			return data->parameters[i].index + 1;
		}
	}
	return 0;
}

struct http_data http_load_data(char* buffer, size_t buffer_size, const char* content_type) {
	char boundary[128];
	http_get_boundary(content_type, boundary);
	struct http_data result;
	result.count = 0;
	result.allocated = 32;
	result.parameters = (struct http_data_part*)malloc(result.allocated * sizeof(struct http_data_part));
	if (!result.parameters) {
		return result;
	}
	const char* name = NULL;
	const char* filename = NULL;
	size_t size = 0;
	char* data = buffer;
	size_t data_size = buffer_size;
	while (data = http_next_form_data(data + size, data_size, boundary, &name, &size, &filename)) {
		data_size = buffer_size - (data - buffer) - size;
		if (result.count >= result.allocated) {
			result.allocated *= 2;
			size_t allocate_size = result.allocated * sizeof(struct http_data_part);
			struct http_data_part* new_parameters = (struct http_data_part*)realloc(result.parameters, allocate_size);
			if (!new_parameters) {
				http_free_data(&result);
				return result;
			}
			result.parameters = new_parameters;
		}
		struct http_data_part* parameter = &result.parameters[result.count];
		parameter->index = http_data_parameter_array_size(&result, name);
		strcpy(parameter->name, name);
		parameter->filename = filename ? filename : "";
		parameter->value = data;
		parameter->size = size;
		result.count++;
	}
	return result;
}

void http_free_data(struct http_data* data) {
	free(data->parameters);
	memset(data, 0, sizeof(struct http_data));
}

struct http_data_part* http_data_param(struct http_data* data, const char* name, int index) {
	for (int i = 0; i < data->count; i++) {
		if (data->parameters[i].index == index && !strcmp(data->parameters[i].name, name)) {
			return &data->parameters[i];
		}
	}
	return NULL;
}

const char* http_data_string(struct http_data* data, const char* name) {
	struct http_data_part* param = http_data_param(data, name, 0);
	return param ? param->value : "";
}

const char* http_data_string_at(struct http_data* data, const char* name, int index) {
	struct http_data_part* param = http_data_param(data, name, index);
	return param ? param->value : "";
}

bool http_read_headers(struct client_state* client) {
	read_from_client(client->socket, &client->buffer, &client->size, &client->allocated, 0);
	const char* end = strstr(client->buffer, "\r\n\r\n");
	if (!end) {
		client->has_headers = false;
		return false;
	}
	end += 4;
	memset(&client->headers, 0, sizeof(struct http_headers));
	http_read_method(client->headers.method, client->buffer);
	http_read_resource(client->headers.resource, client->buffer);
	http_read_content_type(client->headers.content_type, client->buffer);
	http_read_content_length(&client->headers, client->buffer);
	if (strstr(client->buffer, "Connection: keep-alive")) {
		strcpy(client->headers.connection, "keep-alive");
	} else {
		strcpy(client->headers.connection, "close");
	}
	client->has_headers = true;
	client->size -= end - client->buffer;
	memmove(client->buffer, end, client->size);
	client->buffer[client->size] = '\0';
	return true;
}

bool http_read_body(struct client_state* client) {
	if (client->headers.content_length == 0) {
		return true;
	}
	read_from_client(client->socket, &client->buffer, &client->size, &client->allocated, client->headers.content_length);
	return client->size >= client->headers.content_length;
}

void http_write_headers(struct client_state* client, const struct http_headers* headers) {
	char str[1024];
	strcpy(str, "HTTP/1.1 ");
	strcat(str, headers->status);
	strcat(str, "\r\nServer: dmusic\r\nConnection: ");
	strcat(str, headers->connection);
	strcat(str, "\r\nContent-Type: ");
	strcat(str, headers->content_type);
	char length_str[32];
	sprintf(length_str, "\r\nContent-Length: %zu", headers->content_length);
	strcat(str, length_str);
	strcat(str, "\r\n\r\n");
	socket_write_all(client, str, 0);
}

// todo: make two arrays to compare instead
const char* http_file_content_type(const char* path) {
	const char* extension = strrchr(path, '.');
	if (!extension) {
		return "text/plain";
	}
	extension++;
	if (!strcmp(extension, "png")) {
		return "image/png";
	}
	if (!strcmp(extension, "jpg") || !strcmp(extension, "jpeg")) {
		return "image/jpeg";
	}
	if (!strcmp(extension, "js")) {
		return "application/javascript";
	}
	if (!strcmp(extension, "css")) {
		return "text/css";
	}
	if (!strcmp(extension, "html")) {
		return "text/html";
	}
	if (!strcmp(extension, "pdf")) {
		return "application/pdf";
	}
	if (!strcmp(extension, "webp")) {
		return "image/webp";
	}
	if (!strcmp(extension, "svg")) {
		return "image/svg+xml";
	}
	if (!strcmp(extension, "gif")) {
		return "image/gif";
	}
	if (!strcmp(extension, "bmp")) {
		return "image/bmp";
	}
	if (!strcmp(extension, "mp3")) {
		return "audio/mpeg";
	}
	if (!strcmp(extension, "aac")) {
		return "audio/aac";
	}
	if (!strcmp(extension, "webm")) {
		return "video/webm";
	}
	if (!strcmp(extension, "mp4")) {
		return "video/mp4";
	}
	if (!strcmp(extension, "mkv")) {
		return "video/x-matroska";
	}
	if (!strcmp(extension, "zip")) {
		return "application/zip";
	}
	return "text/plain";
}