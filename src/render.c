#include "render.h"
#include "album.h"
#include "cache.h"
#include "config.h"
#include "database.h"
#include "files.h"
#include "format.h"
#include "group.h"
#include "import.h"
#include "session.h"
#include "session_track.h"
#include "system.h"
#include "thumbnail.h"
#include "track.h"
#include "type.h"
#include "upload.h"
#include "user.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	static _Thread_local char key_buffer[128];
	if (strlen(key) >= sizeof(key_buffer)) {
		print_error_f("Parameter key \"%s\" is too long.", key);
		return;
	}
	int key_size = snprintf(key_buffer, 128, "{{ %s }}", key);
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

int get_int_argument(const char** resource) {
	char value[32];
	*resource = split_string(value, sizeof(value), *resource, '/');
	return atoi(value);
}

void render_explore(struct render_buffer* buffer) {
	struct search_data search;
	strcpy(search.name, "explore");
	strcpy(search.type, "explore");
	strcpy(search.value, "");
	strcpy(search.text, "");
	strcpy(search.render, "explore/results");
	strcpy(search.results, "#explore_results");
	render_search(buffer, "search", &search);
}

char* render_resource_with_session(const char* page, const char* resource, const struct cached_session* session) {
	struct render_buffer buffer;
	init_render_buffer(&buffer, 4096);
	if (!strcmp(page, "explore")) {
		assign_buffer(&buffer, get_cached_file("html/explore.html", NULL));
		render_explore(&buffer);
	} else if (!strcmp(page, "recent")) {
		assign_buffer(&buffer, get_cached_file("html/recent.html", NULL));
		render_recent_group_thumbnails(&buffer);
		render_recent_album_thumbnails(&buffer);
	} else if (!strcmp(page, "favourites")) {
		assign_buffer(&buffer, get_cached_file("html/favourites.html", NULL));
		render_favourite_group_thumbnails(&buffer, session->name);
	} else if (!strcmp(page, "groups")) {
		assign_buffer(&buffer, get_cached_file("html/groups.html", NULL));
		render_all_group_thumbnails(&buffer);
	} else if (!strcmp(page, "group")) {
		const int id = get_int_argument(&resource);
		if (id == 0) {
			return buffer.data;
		}
		const bool can_edit = get_preference(session, PREFERENCE_EDIT_MODE) == EDIT_MODE_ON;
		const bool edit_group_tags = can_edit && has_privilege(session, PRIVILEGE_EDIT_GROUP_TAGS);
		const bool edit_details = can_edit && has_privilege(session, PRIVILEGE_ADD_GROUP);
		const bool favourited = is_group_favourited(session->name, id); // todo: cache favourites in session?
		render_group(&buffer, id, edit_group_tags, edit_details, favourited);
	} else if (!strcmp(page, "album") || !strcmp(page, "edit-album")) {
		const int album_release_id = get_int_argument(&resource);
		if (album_release_id == 0) {
			return buffer.data;
		}
		struct album_data album;
		struct track_data* tracks = NULL;
		int num_tracks = 0;
		load_album_release(&album, album_release_id);
		load_album_tracks(&tracks, &num_tracks, album_release_id);
		const bool can_edit = get_preference(session, PREFERENCE_EDIT_MODE) == EDIT_MODE_ON;
		const bool edit_mode = can_edit && has_privilege(session, PRIVILEGE_EDIT_ALBUM_DETAILS);
		if (!strcmp(page, "album")) {
			render_album(&buffer, &album, tracks, num_tracks, edit_mode);
		} else if (can_edit) {
			render_edit_album(&buffer, &album, tracks, num_tracks);
		}
		free(tracks);
		free(album.image);
	} else if (!strcmp(page, "group-tags")) {
		const int id = get_int_argument(&resource);
		if (id == 0) {
			return buffer.data;
		}
		char edit[16];
		resource = split_string(edit, sizeof(edit), resource, '/');
		if (!strcmp(edit, "edit")) {
			render_edit_group_tags(&buffer, id);
		} else {
			render_group_tags(&buffer, id);
		}
	} else if (!strcmp(page, "search")) {
		char type[32];
		char query[512];
		resource = split_string(type, sizeof(type), resource, '/');
		resource = split_string(query, sizeof(query), resource, '/');
		render_search_query(&buffer, type, query);
	} else if (!strcmp(page, "expanded-search")) {
		char type[32];
		char query[512];
		resource = split_string(type, sizeof(type), resource, '/');
		resource = split_string(query, sizeof(query), resource, '/');
		if (!strcmp(type, "explore")) {
			assign_buffer(&buffer, "{{ group-thumbs }}");
			render_group_thumbnails_from_search(&buffer, type, query);
		}
	} else if (!strcmp(page, "upload")) {
		if (has_privilege(session, PRIVILEGE_UPLOAD_ALBUM)) {
			render_upload(&buffer);
		}
	} else if (!strcmp(page, "remote-entries")) {
		if (has_privilege(session, PRIVILEGE_UPLOAD_ALBUM)) {
			render_remote_entries(&buffer);
		}
	} else if (!strcmp(page, "import")) {
		if (has_privilege(session, PRIVILEGE_IMPORT_ALBUM)) {
			char prefix[1024];
			resource = split_string(prefix, sizeof(prefix), resource, '/');
			render_import(&buffer, prefix);
		}
	} else if (!strcmp(page, "import-attachment")) {
		if (has_privilege(session, PRIVILEGE_IMPORT_ALBUM)) {
			char directory[512];
			char filename[512];
			resource = split_string(directory, sizeof(directory), resource, '/');
			resource = split_string(filename, sizeof(filename), resource, '/');
			render_import_attachment(&buffer, directory, filename);
		}
	} else if (!strcmp(page, "session-tracks")) {
		const int from_num = get_int_argument(&resource);
		const int to_num = get_int_argument(&resource);
		render_session_tracks_database(&buffer, session, from_num, to_num);
	} else if (!strcmp(page, "playlists")) {
		append_buffer(&buffer, get_cached_file("html/playlists.html", NULL));
		set_parameter(&buffer, "playlists", "No playlists.");
	} else if (!strcmp(page, "add-group")) {
		if (has_privilege(session, PRIVILEGE_ADD_GROUP)) {
			render_add_group(&buffer);
		}
	} else if (!strcmp(page, "edit-group")) {
		if (has_privilege(session, PRIVILEGE_ADD_GROUP)) { // todo: edit group privilege?
			const int id = get_int_argument(&resource);
			if (id > 0) {
				render_edit_group(&buffer, id);
			}
		}
	} else if (!strcmp(page, "profile")) {
		render_profile(&buffer);
	} else if (!strcmp(page, "import-disc")) {
		assign_buffer(&buffer, get_cached_file("html/import_disc.html", NULL));
		const int num = get_int_argument(&resource);
		set_parameter_int(&buffer, "num", num);
		set_parameter(&buffer, "name", "");
		set_parameter(&buffer, "tracks", "");
	} else if (!strcmp(page, "edit-group-alias")) {
		const int group_id = get_int_argument(&resource);
		char alias[256];
		resource = split_string(alias, sizeof(alias), resource, '/');
		assign_buffer(&buffer, get_cached_file("html/edit_group_alias.html", NULL));
		set_parameter(&buffer, "alias", alias);
		set_parameter_int(&buffer, "group-id", group_id);
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
