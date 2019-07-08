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
			dest->content_length = atoi(buffer);
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
	if (client->headers.content_length <= 0) {
		return true;
	}
	read_from_client(client->socket, &client->buffer, &client->size, &client->allocated, client->headers.content_length);
	if (client->headers.content_length > client->size) {
		return false;
	}
	printf("HTTP Body: \"%s\"\n", client->buffer);
	return true;
}

void http_write_headers(int socket, const struct http_headers* headers) {
	char str[1024];
	strcpy(str, "HTTP/1.1 ");
	strcat(str, headers->status);
	strcat(str, "\r\nServer: dmusic\r\nConnection: ");
	strcat(str, headers->connection);
	strcat(str, "\r\nContent-Type: ");
	strcat(str, headers->content_type);
	if (headers->content_length >= 0) {
		char length_str[64];
		sprintf(length_str, "\r\nContent-Length: %i", headers->content_length);
		strcat(str, length_str);
	}
	strcat(str, "\r\n\r\n");
	socket_write_all(socket, str, -1);
}

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
	return "text/plain";
}
