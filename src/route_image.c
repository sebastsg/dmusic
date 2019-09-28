#include "route.h"
#include "cache.h"
#include "config.h"
#include "format.h"
#include "database.h"

#include <string.h>
#include <stdlib.h>

void route_image(struct route_result* result, const char* resource) {
	if (!resource) {
		return;
	}
	char image[32];
	resource = split_string(image, sizeof(image), resource, '/');
	if (!strcmp(image, "album")) {
		char album_release_id_str[32];
		resource = split_string(album_release_id_str, 32, resource, '/');
		int album_release_id = atoi(album_release_id_str);
		char image_format[32];
		resource = split_string(image_format, 32, resource, '/');
		int num = first_album_attachment_of_type(album_release_id, "cover");
		if (num > 0) {
			route_file(result, server_album_image_path(album_release_id, num));
		}
	} else if (!strcmp(image, "group")) {
		char group_id_str[32];
		resource = split_string(group_id_str, 32, resource, '/');
		int group_id = atoi(group_id_str);
		char image_format[32];
		resource = split_string(image_format, 32, resource, '/');
		int num = first_group_attachment_of_type(group_id, "background");
		if (num > 0) {
			route_file(result, server_group_image_path(group_id, num));
		}
	} else if (!strcmp(image, "flag")) {

	} else if (!strcmp(image, "missing.png")) {

	} else if (!strcmp(image, "bg.jpg")) {
		result->body = get_cached_file("img/bg.jpg", &result->size);
		strcpy(result->type, "image/jpeg");
	}
}
