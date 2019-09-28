#include "route.h"
#include "database.h"
#include "system.h"
#include "http.h"
#include "config.h"
#include "files.h"
#include "format.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int create_group(const char* country, const char* name, const char* website, const char* description) {
	const char* params[] = { country, name, website, description };
	int group_id = insert_row("group", true, 4, params);
	if (group_id > 0) {
		create_directory(server_group_path(group_id));
	}
	return group_id;
}

void create_group_tags(int group_id, const char* comma_separated_tags) {
	char group_id_str[32];
	snprintf(group_id_str, sizeof(group_id_str), "%i", group_id);
	int priority = 1;
	char tag[128];
	while (comma_separated_tags = split_string(tag, sizeof(tag), comma_separated_tags, ',')) {
		char priority_str[32];
		sprintf(priority_str, "%i", priority);
		trim_ends(tag, " \t");
		const char* params[] = { group_id_str, tag, priority_str };
		insert_row("group_tag", false, 3, params);
		priority++;
	}
}

void create_group_member(int group_id, int person_id, const char* role, const char* started_at, const char* ended_at) {
	char group_id_str[32];
	snprintf(group_id_str, sizeof(group_id_str), "%i", group_id);
	char person_id_str[32];
	snprintf(person_id_str, sizeof(person_id_str), "%i", person_id);
	const char* person_params[] = { group_id_str, person_id_str, role, started_at, ended_at };
	insert_row("group_member", false, 5, person_params);
}

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
	char dest_path[512];
	sprintf(dest_path, "%s/%i", server_group_path(group_id), num);
	const char* image_extension = strrchr(url, '.');
	if (image_extension && is_extension_image(image_extension)) {
		strcat(dest_path, image_extension);
	} else {
		print_error_f("Group image %i did not get a file extension.", num);
	}
	size_t image_size = 0;
	char* image_file = download_http_file(url, &image_size);
	write_file(dest_path, image_file, image_size);
	free(image_file);
}

void write_group_attachment_file_uploaded(int group_id, int num, struct http_data* data) {
	struct http_data_part* image_file = http_data_param(data, "image-file", num - 1);
	if (!image_file) {
		return;
	}
	char dest_path[512];
	sprintf(dest_path, "%s/%i", server_group_path(group_id), num);
	const char* image_extension = strrchr(image_file->filename, '.');
	if (image_extension) {
		strcat(dest_path, image_extension);
	}
	write_file(dest_path, image_file->value, image_file->size);
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
