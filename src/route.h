#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "http.h"

struct client_state;
struct http_data;
struct cached_session;

struct route_result {
	struct client_state* client;
	char* body;
	size_t size;
	char type[64];
	bool freeable;
};

struct route_parameters {
	struct route_result* result;
	struct cached_session* session;
	const char* resource;
	char* body;
	size_t size;
	struct http_data data;
};

void initialize_routes();
void free_routes();
void process_route(struct route_result* result, const char* path, char* body, size_t size);
void free_route(struct route_result* result);

// todo: this function should be removed after some refactoring
void set_route_result_html(struct route_result* result, char* body);

void route_file(struct route_result* result, const char* path);

void route_image(struct route_parameters* parameters);
void route_form(struct route_parameters* parameters);
void route_track(struct route_parameters* parameters);
void route_render(struct route_parameters* parameters);
void route_render_main(struct route_parameters* parameters);

void route_form_download_remote(struct http_data* data);
void route_form_attach(struct route_parameters* parameters);
void route_form_import(struct http_data* data);
void route_form_upload(struct http_data* data);
void route_form_add_group(struct http_data* data);
void route_form_add_session_track(struct route_parameters* parameters);
void route_form_delete_session_track(struct route_parameters* parameters);
void route_form_add_group_tag(struct route_result* result, struct http_data* data);
void route_form_delete_group_tag(struct route_result* result, struct http_data* data);
void toggle_favourite_group(struct route_parameters* parameters, int group_id);

void route_form_login(struct route_result* result, struct http_data* data);
void route_form_register(struct http_data* data);
