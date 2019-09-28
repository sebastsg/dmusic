#include "route.h"
#include "format.h"
#include "cache.h"
#include "config.h"
#include "http.h"
#include "network.h"
#include "database.h"
#include "files.h"
#include "stack.h"
#include "session.h"
#include "system.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

void process_route(struct route_result* result, const char* resource, char* body, size_t size) {
	char command[32];
	const char* it = split_string(command, sizeof(command), resource, '/');
	if (strncmp(result->client->headers.session_cookie, "temporary", 9)) {
		route_file(result, server_html_path("login"));
		return;
	}
	if (!strcmp(command, "img")) {
		route_image(result, it);
		return;
	}
	if (!strcmp(command, "track")) {
		route_track(result, it);
		return;
	}
	if (!strcmp(command, "render")) {
		route_render(result, it);
		return;
	}
	if (!strcmp(command, "form")) {
		route_form(result, it, body, size);
		return;
	}
	if (!strcmp(command, "uploaded")) {
		route_file(result, server_uploaded_file_path(it));
		return;
	}
	if (!strcmp(command, "main.js")) {
		result->body = get_cached_file("html/main.js", &result->size);
		strcpy(result->type, "application/javascript");
		return;
	}
	if (!strcmp(command, "style.css")) {
		result->body = get_cached_file("html/style.css", &result->size);
		strcpy(result->type, "text/css");
		return;
	}
	if (!strcmp(command, "icon.png")) {
		result->body = get_cached_file("img/icon.png", &result->size);
		strcpy(result->type, "image/png");
		return;
	}
	route_render_main(result, resource);
}

void free_route(struct route_result* result) {
	if (result->freeable) {
		free(result->body);
		result->freeable = false;
		print_info("Freed route render buffer");
	}
	result->body = NULL;
	result->size = 0;
}
