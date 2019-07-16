#include "route.h"
#include "format.h"
#include "cache.h"
#include "config.h"
#include "http.h"
#include "network.h"
#include "database.h"
#include "files.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void route_file(struct route_result* result, const char* path) {
	printf("Serving file: %s\n", path);
	size_t file_size = 0;
	result->body = read_file(path, &file_size);
	result->size = file_size;
	result->freeable = true;
	strcpy(result->type, http_file_content_type(path));
}

void route_uploaded_file(struct route_result* result, const char* resource) {
	char path[1024];
	route_file(result, server_uploaded_file_path(path, resource));
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
	if (!strcmp(image, "album")) {
		char album_release_id_str[32];
		resource = split_string(album_release_id_str, 32, resource, '/', NULL);
		int album_release_id = atoi(album_release_id_str);
		char image_format[32];
		resource = split_string(image_format, 32, resource, '/', NULL);
		char image_path[512];
		if (!strcmp(image_format, "original")) {
			route_file(result, server_album_image_path(image_path, album_release_id, 1));
		} else {
			route_file(result, server_album_image_path(image_path, album_release_id, 1));
		}
	} else if (!strcmp(image, "group")) {
		char group_id_str[32];
		resource = split_string(group_id_str, 32, resource, '/', NULL);
		int group_id = atoi(group_id_str);
		char image_format[32];
		resource = split_string(image_format, 32, resource, '/', NULL);
		char image_path[512];
		if (!strcmp(image_format, "original")) {
			route_file(result, server_group_image_path(image_path, group_id, 1));
		} else {
			route_file(result, server_group_image_path(image_path, group_id, 1));
		}
	} else if (!strcmp(image, "flag")) {

	} else if (!strcmp(image, "missing.png")) {

	} else if (!strcmp(image, "bg.jpg")) {
		result->body = mem_cache()->bg_jpg;
		result->size = mem_cache()->bg_jpg_size;
		strcpy(result->type, "image/jpeg");
	}
}

static void route_form_add_group(struct http_data* data) {
	const char* name = http_data_string(data, "name");
	const char* country = http_data_string(data, "country");
	const char* website = http_data_string(data, "website");
	const char* description = http_data_string(data, "description");
	const char* params[] = { country, name, website, description };
	int group_id = insert_row("group", true, 4, params);
	if (!group_id) {
		fputs("Failed to add group\n", stderr);
		return;
	}
	char group_id_str[32];
	sprintf(group_id_str, "%i", group_id);
	char directory[256];
	create_directory(group_path(directory, 256, group_id));
	// tags
	int priority = 1;
	char tag[128];
	const char* tags = http_data_string(data, "tags");
	while (tags = split_string(tag, 128, tags, ',', NULL)) {
		char priority_str[32];
		sprintf(priority_str, "%i", priority);
		const char* tag_params[] = { group_id_str, tag, priority_str };
		insert_row("group_tag", false, 3, tag_params);
		priority++;
	}
	// people
	int num_people = http_data_parameter_array_size(data, "person-id");
	if (num_people > 20) {
		return; // seems fishy.
	}
	for (int i = 0; i < num_people; i++) {
		const char* person_id = http_data_string_at(data, "person-id", i);
		const char* role = http_data_string_at(data, "person-role", i);
		const char* started_at = "";
		const char* ended_at = "";
		const char* person_params[] = { group_id_str, person_id, role, started_at, ended_at };
		insert_row("group_member", false, 5, person_params);
	}
	// images
	int num_images = http_data_parameter_array_size(data, "image-source");
	if (num_images > 50) {
		return; // at this point, images should be added afterwards.
	}
	for (int i = 0; i < num_images; i++) {
		const char* source = http_data_string_at(data, "image-source", i);
		const char* is_background = http_data_string_at(data, "image-is-background", i);
		struct http_data_part* image_file = http_data_param(data, "image-file", i);
		if (!image_file) {
			continue;
		}
		const char* image_description = http_data_string_at(data, "image-description", i);
		char dest_path[512];
		sprintf(dest_path, "%s/%i", directory, i + 1);
		const char* image_extension = strrchr(image_file->filename, '.');
		if (image_extension) {
			strcat(dest_path, image_extension);
		}
		if (!strcmp(source, "url")) {
			// todo: download from url
			fputs("Images from URL not supported yet.\n", stderr);
			continue;
		} else if (!strcmp(source, "file")) {
			write_file(dest_path, image_file->value, image_file->size);
		}
		char num_str[32];
		sprintf(num_str, "%i", i + 1);
		const char* image_params[] = { group_id_str, num_str, image_description };
		insert_row("group_image", false, 3, image_params);
	}
}

void route_form(struct route_result* result, const char* resource, char* body, size_t size) {
	if (!resource) {
		return;
	}
	char form[32];
	resource = split_string(form, sizeof(form), resource, '/', NULL);
	struct http_data data = http_load_data(body, size, result->client->headers.content_type);
	if (!strcmp(form, "addgroup")) {
		route_form_add_group(&data);
	}
	http_free_data(&data);
}

void process_route(struct route_result* result, const char* resource, char* body, size_t size) {
	char command[32];
	const char* it = split_string(command, sizeof(command), resource, '/', NULL);
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
	if (!strcmp(command, "uploaded")) {
		route_uploaded_file(result, it);
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
