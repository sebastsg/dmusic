#include "config.h"
#include "database.h"
#include "files.h"
#include "format.h"
#include "group.h"
#include "http.h"
#include "route.h"
#include "stack.h"
#include "system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void create_group_members(int group_id, struct http_data* data) {
	int num_people = http_data_parameter_array_size(data, "person-id");
	for (int i = 0; i < num_people && num_people < 20; i++) {
		const char* person_id = http_data_string_at(data, "person-id", i);
		const char* role = http_data_string_at(data, "person-role", i);
		create_group_member(group_id, atoi(person_id), role, "", "");
	}
}

void write_group_attachment_file_url(int group_id, int num, struct http_data* data) {
	const char* url = http_data_string_at(data, "image-file", num - 1);
	if (!url) {
		return;
	}
	const char* dest_path = push_string_f("%s/%i", server_group_path(group_id), num);
	const char* image_extension = strrchr(url, '.');
	if (image_extension && is_extension_image(image_extension)) {
		dest_path = append_top_string(image_extension);
	} else {
		print_error_f("Group image %i did not get a file extension.", num);
	}
	size_t image_size = 0;
	char* image_file = download_http_file(url, &image_size, "GET", NULL, NULL);
	write_file(dest_path, image_file, image_size);
	free(image_file);
	pop_string();
}

void write_group_attachment_file_uploaded(int group_id, int num, struct http_data* data) {
	struct http_data_part* image_file = http_data_param(data, "image-file", num - 1);
	if (!image_file) {
		return;
	}
	const char* dest_path = push_string_f("%s/%i", server_group_path(group_id), num);
	const char* image_extension = strrchr(image_file->filename, '.');
	if (image_extension) {
		dest_path = append_top_string(image_extension);
	}
	write_file(dest_path, image_file->value, image_file->size);
	pop_string();
}

void create_group_attachment(struct http_data* data, int group_id, int num, const char* source, bool is_background, const char* image_description) {
	if (!strcmp(source, "url")) {
		write_group_attachment_file_url(group_id, num, data);
	} else if (!strcmp(source, "file")) {
		write_group_attachment_file_uploaded(group_id, num, data);
	}
	char attachment_type[16];
	if (is_background) {
		strcpy(attachment_type, "background");
	}
	char group_id_str[32];
	snprintf(group_id_str, sizeof(group_id_str), "%i", group_id);
	char num_str[32];
	sprintf(num_str, "%i", num);
	const char* params[] = { group_id_str, num_str, attachment_type, image_description };
	insert_row("group_attachment", false, 4, params);
}

void create_group_attachments(int group_id, struct http_data* data) {
	int num_images = http_data_parameter_array_size(data, "image-source");
	for (int i = 0; i < num_images && num_images < 50; i++) {
		const char* source = http_data_string_at(data, "image-source", i);
		const char* is_background = http_data_string_at(data, "image-is-background", i);
		const char* image_description = http_data_string_at(data, "image-description", i);
		create_group_attachment(data, group_id, i + 1, source, is_background != NULL, image_description);
	}
}

void route_form_add_group(struct http_data* data) {
	const char* country = http_data_string(data, "country");
	const char* name = http_data_string(data, "name");
	const char* website = http_data_string(data, "website");
	const char* description = http_data_string(data, "description");
	int group_id = create_group(country, name, website, description);
	if (group_id > 0) {
		create_group_tags(group_id, http_data_string(data, "tags"));
		create_group_members(group_id, data);
		create_group_attachments(group_id, data);
	} else {
		print_error("Failed to add group.");
	}
}
