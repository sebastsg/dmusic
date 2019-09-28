#pragma once

#include <stdbool.h>
#include <stddef.h>

struct client_state;
struct http_data;

struct route_result {
	struct client_state* client;
	char* body;
	size_t size;
	char type[64];
	bool freeable;
};

void process_route(struct route_result* result, const char* path, char* body, size_t size);
void free_route(struct route_result* result);

void route_image(struct route_result* result, const char* resource);
void route_form(struct route_result* result, const char* resource, char* body, size_t size);
void route_form_download_remote(struct http_data* data);
void route_form_attach(struct route_result* result, struct http_data* data);
void route_form_import(struct http_data* data);
void route_form_upload(struct http_data* data);
void route_form_add_group(struct http_data* data);
void route_form_add_session_track(struct route_result* result, struct http_data* data);
void route_track(struct route_result* result, const char* resource);
void route_render(struct route_result* result, const char* resource);
void route_file(struct route_result* result, const char* path);
void route_render_main(struct route_result* result, const char* resource);
