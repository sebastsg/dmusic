#include "route.h"
#include "format.h"
#include "cache.h"
#include "config.h"
#include "network.h"
#include "database.h"
#include "files.h"
#include "stack.h"
#include "session.h"
#include "system.h"
#include "generic.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define ROUTE_NAME_SIZE 32

typedef void (*route_handler_function)(struct route_parameters*);

struct route_handler {
	route_handler_function function;
	char name[ROUTE_NAME_SIZE];
};

struct route_handler_list {
	struct route_handler* handlers;
	int count;
	int allocated;
};

static struct route_handler_list routes;

void route_uploaded(struct route_parameters* parameters) {
	if (parameters->session) {
		route_file(parameters->result, server_uploaded_file_path(parameters->resource));
	} else {
		print_info("Session required to view uploaded files.");
	}
}

void route_main_js(struct route_parameters* parameters) {
	parameters->result->body = get_cached_file("html/main.js", &parameters->result->size);
	strcpy(parameters->result->type, "application/javascript");
}

void route_style_css(struct route_parameters* parameters) {
	parameters->result->body = get_cached_file("html/style.css", &parameters->result->size);
	strcpy(parameters->result->type, "text/css");
}

void route_icon_png(struct route_parameters* parameters) {
	parameters->result->body = get_cached_file("img/icon.png", &parameters->result->size);
	strcpy(parameters->result->type, "image/png");
}

void add_route(const char* name, route_handler_function function) {
	enum resize_status status = resize_array((void**)&routes.handlers, sizeof(struct route_handler), &routes.allocated, routes.count + 1);
	if (status == RESIZE_FAILED) {
		print_error_f("Failed to add %s to route handler list. Resize failed.", name);
		return;
	}
	struct route_handler* handler = &routes.handlers[routes.count];
	handler->function = function;
	strcpy(handler->name, name);
	routes.count++;
}

void initialize_routes() {
	memset(&routes, 0, sizeof(routes));
	resize_array((void**)&routes.handlers, sizeof(struct route_handler), &routes.allocated, 16);
	add_route("img", route_image);
	add_route("render", route_render);
	add_route("track", route_track);
	add_route("form", route_form);
	add_route("uploaded", route_uploaded);
	add_route("main.js", route_main_js);
	add_route("style.css", route_style_css);
	add_route("icon.png", route_icon_png);
	add_route("favicon.ico", route_icon_png);
}

void free_routes() {
	free(routes.handlers);
	memset(&routes, 0, sizeof(routes));
}

// todo: this function should be removed after some refactoring
void set_route_result_html(struct route_result* result, char* body) {
	result->body = body;
	if (result->body) {
		strcpy(result->type, "text/html");
		result->size = strlen(result->body);
		result->freeable = true;
	}
}

void process_route(struct route_result* result, const char* resource, char* body, size_t size) {
	char route_name[ROUTE_NAME_SIZE];
	struct route_parameters parameters;
	parameters.result = result;
	parameters.session = open_session(result->client->headers.session_cookie);
	parameters.resource = split_string(route_name, sizeof(route_name), resource, '/');
	parameters.body = body;
	parameters.size = size;
	for (int i = 0; i < routes.count; i++) {
		if (!strcmp(routes.handlers[i].name, route_name)) {
			routes.handlers[i].function(&parameters);
			return;
		}
	}
	parameters.resource = resource;
	route_render_main(&parameters);
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
