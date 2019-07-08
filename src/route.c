#include "route.h"
#include "format.h"
#include "cache.h"
#include "config.h"
#include "http.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void route_file(struct route_result* result, const char* resource) {
	
	//size_t file_size = 0;
	//result->body = read_file(absolute_path, &file_size);
	//result->size = (int)file_size;
	//result->freeable = true;
	//result->type = http_file_content_type(resource);
}

void route_render(struct route_result* result, const char* resource) {
	result->body = render_resource(false, resource);
	if (result->body) {
		strcpy(result->type, "text/html");
		result->size = strlen(result->body);
		result->freeable = true;
	}
}

void route_render_main(struct route_result* result, const char* page, const char* resource) {
	if (!page) {
		return;
	}
	if (strlen(page) == 0) {
		resource = "playlists";
	}
	result->body = render_resource(true, resource);
	if (result->body) {
		strcpy(result->type, "text/html");
		result->size = strlen(result->body);
		result->freeable = true;
	}
}

void route_image(struct route_result* result, const char* resource) {
	if (!resource) {
		return;
	}
	char image[32];
	resource = split_string(image, sizeof(image), resource, '/', NULL);
	if (!strcmp(image, "flag")) {

	}
	if (!strcmp(image, "missing.png")) {

	}
	if (!strcmp(image, "bg.jpg")) {
		result->body = mem_cache()->bg_jpg;
		result->size = mem_cache()->bg_jpg_size;
		strcpy(result->type, "image/jpeg");
	}
}

void route_form(struct route_result* result, const char* resource, const char* body, int size) {

}

void process_route(struct route_result* result, const char* resource, const char* body, int size) {
	memset(result, 0, sizeof(struct route_result));
	char command[32];
	const char* it = split_string(command, 32, resource, '/', NULL);
	if (!strcmp(command, "img")) {
		route_image(result, it);
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
	if (!strcmp(command, "main.js")) {
		result->body = mem_cache()->main_js;
		result->size = strlen(result->body);
		strcpy(result->type, "application/javascript");
		return;
	}
	if (!strcmp(command, "style.css")) {
		result->body = mem_cache()->style_css;
		result->size = strlen(result->body);
		strcpy(result->type, "text/css");
		return;
	}
	if (!strcmp(command, "icon.png")) {
		result->body = mem_cache()->icon_png;
		result->size = mem_cache()->icon_png_size;
		strcpy(result->type, "image/png");
		return;
	}
	route_render_main(result, command, it);
}

void free_route(struct route_result* result) {
	if (result->freeable) {
		free(result->body);
		result->body = NULL;
		result->size = 0;
	}
}
