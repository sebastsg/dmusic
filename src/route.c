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

static struct route_handler_list handler_list;

void route_uploaded(struct route_parameters* parameters) {
	if (!parameters->session) {
		print_info("Session required to view uploaded files.");
		return;
	}
	route_file(parameters->result, server_uploaded_file_path(parameters->resource));
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
	enum resize_status status = resize_array((void**)&handler_list.handlers, sizeof(struct route_handler), &handler_list.allocated, handler_list.count + 1);
	if (status == RESIZE_FAILED) {
		print_error_f("Failed to add %s to route handler list Resize failed.", name);
		return;
	}
	struct route_handler* handler = &handler_list.handlers[handler_list.count];
	handler->function = function;
	strcpy(handler->name, name);
	handler_list.count++;
}

void initialize_routes() {
	memset(&handler_list, 0, sizeof(handler_list));
	resize_array((void**)&handler_list.handlers, sizeof(struct route_handler), &handler_list.count, 16);
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
	free(handler_list.handlers);
	memset(&handler_list, 0, sizeof(handler_list));
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
	for (int i = 0; i < handler_list.count; i++) {
		if (!strcmp(handler_list.handlers[i].name, route_name)) {
			print_info_f("Route: %s.", route_name);
			handler_list.handlers[i].function(&parameters);
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
