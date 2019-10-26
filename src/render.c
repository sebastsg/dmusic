#include "render.h"
#include "type.h"
#include "format.h"
#include "config.h"
#include "files.h"
#include "database.h"
#include "cache.h"
#include "session.h"
#include "system.h"
#include "session_track.h"
#include "group.h"
#include "import.h"
#include "user.h"
#include "upload.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void resize_render_buffer(struct render_buffer* buffer, int required_size) {
	if (buffer->size >= required_size) {
		return;
	}
	required_size *= 2; // prevent small reallocations
	if (!buffer->data) {
		buffer->data = (char*)malloc(required_size);
		if (buffer->data) {
			buffer->data[0] = '\0';
			buffer->size = required_size;
			print_info_f("Allocated %i bytes to render buffer.", required_size);
		} else {
			print_error_f("Failed to allocate %i bytes to render buffer.", required_size);
		}
		return;
	}
	char* new_data = (char*)realloc(buffer->data, required_size);
	if (new_data) {
		buffer->data = new_data;
		buffer->size = required_size;
		print_info_f("Resized render buffer to %i bytes.", required_size);
	} else {
		free(buffer->data);
		buffer->data = NULL;
		buffer->size = 0;
		print_error_f("Failed to resize render buffer to %i bytes.", required_size);
	}
}

void init_render_buffer(struct render_buffer* buffer, int size) {
	memset(buffer, 0, sizeof(*buffer));
	resize_render_buffer(buffer, size);
}

void assign_buffer(struct render_buffer* buffer, const char* str) {
	resize_render_buffer(buffer, strlen(str) + 1);
	strcpy(buffer->data, str);
}

void append_buffer(struct render_buffer* buffer, const char* str) {
	resize_render_buffer(buffer, strlen(buffer->data) + strlen(str) + 1);
	strcat(buffer->data, str);
}

void set_parameter(struct render_buffer* buffer, const char* key, const char* value) {
	static char key_buffer[128];
	if (strlen(key) >= sizeof(key_buffer)) {
		print_error_f("Parameter key \"%s\" is too long.", key);
		return;
	}
	int key_size = sprintf(key_buffer, "{{ %s }}", key);
	char* found = strstr(buffer->data + buffer->offset, key_buffer);
	if (found) {
		int found_offset = (found - buffer->data);
		int value_size = (value ? strlen(value) : 0);
		resize_render_buffer(buffer, found_offset + value_size + strlen(found));
		found = buffer->data + found_offset;
		replace_substring(buffer->data, found, found + key_size, buffer->size, value ? value : "");
		buffer->offset = (found - buffer->data);
	} else {
		print_error_f("Failed to set parameter \"%s\".", key);
	}
}

void set_parameter_int(struct render_buffer* buffer, const char* key, int value) {
	char value_string[32];
	sprintf(value_string, "%i", value);
	set_parameter(buffer, key, value_string);
}

char* render_resource_with_session(const char* page, const char* resource, const struct cached_session* session) {
	struct render_buffer buffer;
	init_render_buffer(&buffer, 4096);
	if (!strcmp(page, "groups")) {
		render_group_list(&buffer);
	} else if (!strcmp(page, "group")) {
		char group_id_str[32];
		resource = split_string(group_id_str, sizeof(group_id_str), resource, '/');
		int id = atoi(group_id_str);
		if (id == 0) {
			return buffer.data;
		}
		render_group(&buffer, id);
	} else if (!strcmp(page, "search")) {
		char type[32];
		char query[512];
		resource = split_string(type, sizeof(type), resource, '/');
		resource = split_string(query, sizeof(query), resource, '/');
		render_search_query(&buffer, type, query);
	} else if (!strcmp(page, "upload")) {
		render_upload(&buffer);
	} else if (!strcmp(page, "import")) {
		char prefix[1024];
		resource = split_string(prefix, sizeof(prefix), resource, '/');
		render_import(&buffer, prefix);
	} else if (!strcmp(page, "import_attachment")) {
		char directory[512];
		char filename[512];
		resource = split_string(directory, sizeof(directory), resource, '/');
		resource = split_string(filename, sizeof(filename), resource, '/');
		render_import_attachment(&buffer, directory, filename);
	} else if (!strcmp(page, "session_tracks")) {
		char num_str[32];
		resource = split_string(num_str, sizeof(num_str), resource, '/');
		int from_num = atoi(num_str);
		resource = split_string(num_str, sizeof(num_str), resource, '/');
		int to_num = atoi(num_str);
		render_session_tracks_database(&buffer, session, from_num, to_num);
	} else if (!strcmp(page, "playlists")) {
		append_buffer(&buffer, get_cached_file("html/playlists.html", NULL));
		set_parameter(&buffer, "playlists", "No playlists.");
	} else if (!strcmp(page, "add_group")) {
		render_add_group(&buffer);
	} else if (!strcmp(page, "profile")) {
		render_profile(&buffer);
	} else {
		print_error_f("Cannot render unknown page: %s", page);
	}
	return buffer.data;
}

char* render_resource_without_session(const char* page, const char* resource) {
	struct render_buffer buffer;
	init_render_buffer(&buffer, 4096);
	if (page[0] == '\0' || !strcmp(page, "login")) {
		render_login(&buffer);
	} else if (!strcmp(page, "register")) {
		render_register(&buffer);
	} else {
		print_error_f("Cannot render unknown page: %s", page);
	}
	return buffer.data;
}

char* render_resource(const char* resource, const struct cached_session* session) {
	char page[64];
	resource = split_string(page, sizeof(page), resource, '/');
	if (session) {
		return render_resource_with_session(page, resource, session);
	} else {
		return render_resource_without_session(page, resource);
	}
}
