#include <stdlib.h>
#include <string.h>

#include "cache.h"
#include "config.h"
#include "database.h"
#include "files.h"
#include "format.h"
#include "route.h"
#include "stack.h"
#include "system.h"

static void create_thumbnail(const char* source, const char* destination) {
	if (file_exists(destination)) {
		return;
	}
	if (!file_exists(source)) {
		print_error_f("Did not find source image: " A_CYAN "%s", source);
		return;
	}
	char resize[16];
	strcpy(resize, "320x320>");
	push_string_f("convert \"%s\" -thumbnail \"%s\" \"%s\"", source, resize, destination);
	system_execute(top_string());
	pop_string();
}

void route_image(struct route_parameters* parameters) {
	if (!parameters->resource || !parameters->session) {
		return;
	}
	char image[32];
	parameters->resource = split_string(image, sizeof(image), parameters->resource, '/');
	if (!strcmp(image, "album")) {
		char album_release_id_str[32];
		parameters->resource = split_string(album_release_id_str, 32, parameters->resource, '/');
		int album_release_id = atoi(album_release_id_str);
		char attachment_num[32];
		parameters->resource = split_string(attachment_num, 32, parameters->resource, '/');
		int num = first_album_attachment_of_type(album_release_id, "cover");
		if (num > 0) {
			const char* image_path = push_string(server_album_image_path(album_release_id, num, false));
			char option[32];
			parameters->resource = split_string(option, 32, parameters->resource, '/');
			bool use_thumbnail = strcmp(option, "thumbnail") == 0;
			if (use_thumbnail) {
				const char* thumb_path = server_album_image_path(album_release_id, num, true);
				create_thumbnail(image_path, thumb_path);
				image_path = thumb_path;
			}
			route_file(parameters->result, image_path);
			pop_string();
		}
	} else if (!strcmp(image, "group")) {
		char group_id_str[32];
		parameters->resource = split_string(group_id_str, 32, parameters->resource, '/');
		int group_id = atoi(group_id_str);
		char attachment_num[32];
		parameters->resource = split_string(attachment_num, 32, parameters->resource, '/');
		int num = first_group_attachment_of_type(group_id, "background");
		if (num > 0) {
			const char* image_path = push_string(server_group_image_path(group_id, num, false));
			char option[32];
			parameters->resource = split_string(option, 32, parameters->resource, '/');
			bool use_thumbnail = strcmp(option, "thumbnail") == 0;
			if (use_thumbnail) {
				const char* thumb_path = server_group_image_path(group_id, num, true);
				create_thumbnail(image_path, thumb_path);
				image_path = thumb_path;
			}
			route_file(parameters->result, image_path);
			pop_string();
		}
	} else if (!strcmp(image, "flag")) {
	} else if (!strcmp(image, "missing.png")) {
		parameters->result->body = get_cached_file("img/missing.png", &parameters->result->size);
		strcpy(parameters->result->type, "image/png");
	} else if (!strcmp(image, "bg.jpg")) {
		parameters->result->body = get_cached_file("img/bg.jpg", &parameters->result->size);
		strcpy(parameters->result->type, "image/jpeg");
	}
}
