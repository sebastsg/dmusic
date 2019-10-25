#include "route.h"
#include "cache.h"
#include "config.h"
#include "format.h"
#include "database.h"

#include <string.h>
#include <stdlib.h>

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
		char image_format[32];
		parameters->resource = split_string(image_format, 32, parameters->resource, '/');
		int num = first_album_attachment_of_type(album_release_id, "cover");
		if (num > 0) {
			route_file(parameters->result, server_album_image_path(album_release_id, num));
		}
	} else if (!strcmp(image, "group")) {
		char group_id_str[32];
		parameters->resource = split_string(group_id_str, 32, parameters->resource, '/');
		int group_id = atoi(group_id_str);
		char image_format[32];
		parameters->resource = split_string(image_format, 32, parameters->resource, '/');
		int num = first_group_attachment_of_type(group_id, "background");
		if (num > 0) {
			route_file(parameters->result, server_group_image_path(group_id, num));
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
