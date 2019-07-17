#include "data.h"
#include "format.h"
#include "config.h"
#include "files.h"
#include "database.h"
#include "cache.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct render_buffer {
	char* data;
	int size;
	int offset;
};

static void resize_render_buffer(struct render_buffer* buffer, int required_size) {
	if (buffer->size >= required_size) {
		return;
	}
	required_size *= 2; // prevent small reallocations
	if (!buffer->data) {
		buffer->data = (char*)malloc(required_size);
		if (buffer->data) {
			buffer->data[0] = '\0';
			buffer->size = required_size;
		} else {
			fprintf(stderr, "Memory allocation failed.\n");
		}
		return;
	}
	char* new_data = (char*)realloc(buffer->data, required_size);
	if (new_data) {
		buffer->data = new_data;
		buffer->size = required_size;
	} else {
		free(buffer->data);
		buffer->data = NULL;
		buffer->size = 0;
		fprintf(stderr, "Memory reallocation failed.\n");
	}
}

static void init_render_buffer(struct render_buffer* buffer, int size) {
	memset(buffer, 0, sizeof(*buffer));
	resize_render_buffer(buffer, size);
}

static void set_parameter(struct render_buffer* buffer, const char* key, const char* value) {
	static char key_buffer[128];
	if (strlen(key) >= sizeof(key_buffer)) {
		fprintf(stderr, "Parameter key \"%s\" is too long.\n", key);
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
		fprintf(stderr, "Failed to set parameter \"%s\".\n", key);
	}
}

static void set_parameter_int(struct render_buffer* buffer, const char* key, int value) {
	char value_string[32];
	sprintf(value_string, "%i", value);
	set_parameter(buffer, key, value_string);
}

static void render_search(struct render_buffer* buffer, const char* key, struct search_data* search) {
	set_parameter(buffer, key, mem_cache()->search_template);
	set_parameter(buffer, "name", search->name);
	set_parameter(buffer, "value", search->value);
	set_parameter(buffer, "type", search->type);
	set_parameter(buffer, "text", search->text);
}

static void render_search_results(struct render_buffer* buffer, struct search_result_data* results, int num_results) {
	struct render_buffer result_buffer;
	init_render_buffer(&result_buffer, 2048);
	for (int i = 0; i < num_results; i++) {
		strcat(result_buffer.data, mem_cache()->search_result_template);
		set_parameter(&result_buffer, "value", results[i].value);
		set_parameter(&result_buffer, "text", results[i].text);
	}
	strcpy(buffer->data, "{{ results }}");
	set_parameter(buffer, "results", result_buffer.data);
	free(result_buffer.data);
}

static void render_select_option(struct render_buffer* buffer, const char* key, const char* value, const char* text, bool selected) {
	set_parameter(buffer, key, mem_cache()->select_option_template);
	set_parameter(buffer, "value", value);
	set_parameter(buffer, "selected", selected ? "selected" : "");
	set_parameter(buffer, "text", text);
}

static void render_session_track(struct render_buffer* buffer, const char* key, struct session_track_data* track) {
	set_parameter(buffer, key, mem_cache()->session_track_template);
	set_parameter_int(buffer, "album", track->album_release_id);
	set_parameter_int(buffer, "disc", track->disc_num);
	set_parameter_int(buffer, "track", track->track_num);
	set_parameter(buffer, "name", track->name);
	set_parameter(buffer, "duration", track->duration);
}

static void render_session_tracks(struct render_buffer* buffer, const char* key, struct session_track_data* tracks, int num_tracks) {
	struct render_buffer item_buffer;
	init_render_buffer(&item_buffer, 512);
	for (int i = 0; i < num_tracks; i++) {
		strcat(item_buffer.data, "{{ track }}");
		render_session_track(&item_buffer, "track", &tracks[i]);
	}
	set_parameter(buffer, key, item_buffer.data);
	free(item_buffer.data);
}

static void render_group_thumb(struct render_buffer* buffer, const char* key, int id, const char* name, const char* image) {
	set_parameter(buffer, key, mem_cache()->group_thumb_template);
	set_parameter_int(buffer, "id", id);
	set_parameter(buffer, "image", image);
	set_parameter(buffer, "name", name);
}

static void render_group_thumb_list(struct render_buffer* buffer, struct group_thumb_data* thumbs, int num_thumbs) {
	struct render_buffer item_buffer;
	init_render_buffer(&item_buffer, 512);
	for (int i = 0; i < num_thumbs; i++) {
		strcat(item_buffer.data, "{{ thumb }}");
		render_group_thumb(&item_buffer, "thumb", thumbs[i].id, thumbs[i].name, thumbs[i].image);
	}
	strcpy(buffer->data, mem_cache()->group_thumb_list_template);
	set_parameter(buffer, "thumbs", item_buffer.data);
	free(item_buffer.data);
}

static void render_tags(struct render_buffer* buffer, const char* key, struct tag_data* tags, int num_tags) {
	struct render_buffer item_buffer;
	init_render_buffer(&item_buffer, 512);
	for (int i = 0; i < num_tags; i++) {
		strcat(item_buffer.data, "{{ tag }}");
		set_parameter(&item_buffer, "tag", mem_cache()->tag_link_template);
		set_parameter(&item_buffer, "tag_link", tags[i].name);
		set_parameter(&item_buffer, "tag_name", tags[i].name);
	}
	set_parameter(buffer, key, item_buffer.data);
	free(item_buffer.data);
}

static void render_track(struct render_buffer* buffer, const char* key, struct track_data* track) {
	set_parameter(buffer, key, mem_cache()->track_template);
	char track_url[64];
	client_track_path(track_url, "mp3-320", track->album_release_id, track->disc_num, track->num);
	set_parameter(buffer, "queue_url", track_url);
	set_parameter(buffer, "space_prefix", track->num < 10 ? "&nbsp;" : "");
	set_parameter_int(buffer, "num_str", track->num);
	set_parameter(buffer, "play_url", track_url);
	set_parameter(buffer, "name", track->name);
}

static void render_disc(struct render_buffer* buffer, const char* key, int album_release_id, int disc_num, struct track_data* tracks, int num_tracks, int* track_offset, bool* last_track) {
	struct render_buffer tracks_buffer;
	init_render_buffer(&tracks_buffer, 2048);
	while (*track_offset < num_tracks) {
		if (tracks[*track_offset].album_release_id != album_release_id) {
			*last_track = true;
			break;
		}
		strcat(tracks_buffer.data, "{{ track }}");
		render_track(&tracks_buffer, "track", &tracks[*track_offset]);
		(*track_offset)++;
	}
	set_parameter(buffer, key, mem_cache()->disc_template);
	set_parameter(buffer, "tracks", tracks_buffer.data);
	free(tracks_buffer.data);
	if (*track_offset == num_tracks) {
		*last_track = true;
	}
}

static void render_album(struct render_buffer* buffer, const char* key, struct album_data* album, struct track_data* tracks, int num_tracks, int* track_offset) {
	struct render_buffer discs_buffer;
	init_render_buffer(&discs_buffer, 2048);
	bool last_track = false;
	int i = 1;
	while (!last_track) {
		strcat(discs_buffer.data, "{{ disc }}");
		render_disc(&discs_buffer, "disc", album->album_release_id, i, tracks, num_tracks, track_offset, &last_track);
		i++;
	}
	set_parameter(buffer, key, mem_cache()->album_template);
	set_parameter(buffer, "image", album->image);
	set_parameter_int(buffer, "id_name", album->id);
	set_parameter(buffer, "name", album->name);
	set_parameter_int(buffer, "id_play", album->id);
	set_parameter_int(buffer, "id_queue", album->id);
	set_parameter_int(buffer, "id_download", album->id);
	set_parameter(buffer, "released", album->released_at);
	set_parameter(buffer, "discs", discs_buffer.data);
	free(discs_buffer.data);
}

static void render_albums(struct render_buffer* buffer, const char* key, const char* type, const char* name, struct album_data* albums, int num_albums, struct track_data* tracks, int num_tracks) {
	struct render_buffer item_buffer;
	init_render_buffer(&item_buffer, 2048);
	int track_offset = 0;
	int count = 0;
	for (int i = 0; i < num_albums; i++) {
		if (strcmp(type, albums[i].album_type_code)) {
			continue;
		}
		strcat(item_buffer.data, "{{ album }}");
		render_album(&item_buffer, "album", &albums[i], tracks, num_tracks, &track_offset);
		count++;
	}
	if (count > 0) {
		set_parameter(buffer, key, mem_cache()->albums_template);
		set_parameter(buffer, "title", name);
		set_parameter(buffer, "albums", item_buffer.data);
	} else {
		set_parameter(buffer, key, "");
	}
	free(item_buffer.data);
}

static void render_group(struct render_buffer* buffer, struct group_data* group) {
	strcpy(buffer->data, "{{ group }}");
	set_parameter(buffer, "group", mem_cache()->group_template);
	set_parameter(buffer, "name", group->name);
	render_tags(buffer, "tags", group->tags, group->num_tags);
	struct render_buffer album_list_buffer;
	init_render_buffer(&album_list_buffer, 2048);
	for (int i = 0; i < mem_cache()->album_types.count; i++) {
		struct select_option* option = &mem_cache()->album_types.options[i];
		strcat(album_list_buffer.data, "{{ albums }}");
		render_albums(&album_list_buffer, "albums", option->code, option->name, group->albums, group->num_albums, group->tracks, group->num_tracks);
	}
	set_parameter(buffer, "albums", album_list_buffer.data);
	free(album_list_buffer.data);
}

static void render_upload(struct render_buffer* buffer, struct upload_data* upload) {
	strcpy(buffer->data, "{{ uploads }}");
	set_parameter(buffer, "uploads", mem_cache()->uploaded_albums_template);
	struct render_buffer uploads_buffer;
	init_render_buffer(&uploads_buffer, 2048);
	for (int i = 0; i < upload->num_uploads; i++) {
		strcat(uploads_buffer.data, mem_cache()->uploaded_album_template);
		set_parameter(&uploads_buffer, "prefix", upload->uploads[i].prefix);
		set_parameter(&uploads_buffer, "name", upload->uploads[i].name);
	}
	set_parameter(buffer, "uploads", uploads_buffer.data);
	free(uploads_buffer.data);
}

static void render_prepare_disc(struct render_buffer* buffer, const char* key, struct prepare_disc_data* disc) {
	set_parameter(buffer, key, mem_cache()->prepare_disc_template);
	set_parameter_int(buffer, "num", disc->num);
	set_parameter(buffer, "name", disc->name);
	struct render_buffer tracks_buffer;
	init_render_buffer(&tracks_buffer, 4096);
	for (int i = 0; i < disc->num_tracks; i++) {
		// todo: fix potential security issue where a path has {{ fixed }} etc...
		strcat(tracks_buffer.data, mem_cache()->prepare_track_template);
		set_parameter_int(&tracks_buffer, "num", disc->tracks[i].num);
		set_parameter(&tracks_buffer, "path", disc->tracks[i].path);
		set_parameter(&tracks_buffer, "fixed", disc->tracks[i].fixed);
		set_parameter(&tracks_buffer, "uploaded", disc->tracks[i].uploaded);
		set_parameter(&tracks_buffer, "extension", disc->tracks[i].extension);
	}
	set_parameter(buffer, "tracks", tracks_buffer.data);
	free(tracks_buffer.data);
}

static void render_prepare_attachments(struct render_buffer* buffer, const char* key, struct prepare_attachment_data* attachments, int num_attachments) {
	struct render_buffer attachments_buffer;
	init_render_buffer(&attachments_buffer, 4096);
	for (int i = 0; i < num_attachments; i++) {
		strcat(attachments_buffer.data, mem_cache()->prepare_attachment_template);
		const char* extension = strrchr(attachments[i].name, '.');
		const char* blacklist[] = { ".sfv", ".m3u", ".nfo" };
		bool checked = true;
		for (int j = 0; extension && j < 3; j++) {
			if (!strcmp(extension, blacklist[j])) {
				checked = false;
				break;
			}
		}
		set_parameter(&attachments_buffer, "checked", checked ? "checked" : "");
		set_parameter(&attachments_buffer, "path", attachments[i].path);
		
		struct render_buffer select_buffer;
		init_render_buffer(&select_buffer, 2048);
		for (int j = 0; j < attachments[i].targets.count; j++) {
			struct select_option* target = &attachments[i].targets.options[j];
			strcat(select_buffer.data, "{{ option }}");
			render_select_option(&select_buffer, "option", target->code, target->name, j == attachments[i].selected_target);
		}
		set_parameter(&attachments_buffer, "targets", select_buffer.data);
		free(select_buffer.data);

		set_parameter(&attachments_buffer, "link", attachments[i].link);
		set_parameter(&attachments_buffer, "name", attachments[i].name);
		set_parameter(&attachments_buffer, "link", attachments[i].link);
		set_parameter(&attachments_buffer, "preview", attachments[i].preview);
	}
	set_parameter(buffer, key, attachments_buffer.data);
	free(attachments_buffer.data);
}

static void render_prepare(struct render_buffer* buffer, struct prepare_data* prepare) {
	if (prepare->num_discs < 1 || prepare->discs[0].num_tracks < 1) {
		return;
	}
	strcpy(buffer->data, mem_cache()->prepare_template);
	set_parameter(buffer, "filename", prepare->filename);
	set_parameter(buffer, "name", prepare->album_name);
	set_parameter(buffer, "released_at", prepare->released_at);
	render_search(buffer, "group_search", &prepare->group_search);
	struct render_buffer select_buffer;
	init_render_buffer(&select_buffer, 2048);
	const char* extension = prepare->discs[0].tracks[0].extension;
	for (int i = 0; i < mem_cache()->audio_formats.count; i++) {
		struct select_option* format = &mem_cache()->audio_formats.options[i];
		if (strstr(format->name, "FLAC") && strcmp(extension, "flac")) {
			continue;
		}
		if (strstr(format->name, "MP3") && strcmp(extension, "mp3")) {
			continue;
		}
		if (strstr(format->name, "AAC") && strcmp(extension, "m4a")) {
			continue;
		}
		strcat(select_buffer.data, "{{ option }}");
		bool selected = !strcmp(prepare->audio_format, format->code);
		render_select_option(&select_buffer, "option", format->code, format->name, selected);
	}
	set_parameter(buffer, "formats", select_buffer.data);
	select_buffer.offset = 0;
	select_buffer.data[0] = '\0';
	for (int i = 0; i < mem_cache()->album_types.count; i++) {
		struct select_option* type = &mem_cache()->album_types.options[i];
		strcat(select_buffer.data, "{{ option }}");
		bool selected = !strcmp(prepare->album_type, type->code);
		render_select_option(&select_buffer, "option", type->code, type->name, selected);
	}
	set_parameter(buffer, "types", select_buffer.data);
	select_buffer.offset = 0;
	select_buffer.data[0] = '\0';
	for (int i = 0; i < mem_cache()->album_release_types.count; i++) {
		struct select_option* type = &mem_cache()->album_release_types.options[i];
		strcat(select_buffer.data, "{{ option }}");
		bool selected = !strcmp(prepare->album_release_type, type->code);
		render_select_option(&select_buffer, "option", type->code, type->name, selected);
	}
	set_parameter(buffer, "release_types", select_buffer.data);
	render_prepare_attachments(buffer, "attachments", prepare->attachments, prepare->num_attachments);
	set_parameter(buffer, "folder", prepare->folder);
	struct render_buffer discs_buffer;
	init_render_buffer(&discs_buffer, 8192);
	for (int i = 0; i < prepare->num_discs; i++) {
		strcat(discs_buffer.data, "{{ disc }}");
		render_prepare_disc(&discs_buffer, "disc", &prepare->discs[i]);
	}
	set_parameter(buffer, "discs", discs_buffer.data);
	free(discs_buffer.data);
	free(select_buffer.data);
}

static void render_add_group(struct render_buffer* buffer, struct add_group_data* add_group) {
	strcpy(buffer->data, "{{ add_group }}");
	set_parameter(buffer, "add_group", mem_cache()->add_group_template);
	render_search(buffer, "country_search", &add_group->country);
	render_search(buffer, "person_search", &add_group->person);
	struct render_buffer select_buffer;
	init_render_buffer(&select_buffer, 2048);
	for (int i = 0; i < mem_cache()->roles.count; i++) {
		struct select_option* role = &mem_cache()->roles.options[i];
		strcat(select_buffer.data, "{{ option }}");
		render_select_option(&select_buffer, "option", role->code, role->name, 0);
	}
	set_parameter(buffer, "roles", select_buffer.data);
	free(select_buffer.data);
}

static void render_profile(struct render_buffer* buffer) {
	strcpy(buffer->data, mem_cache()->profile_template);
}

static void render_main(struct render_buffer* buffer, const char* resource) {
	char user[64];
	strcpy(user, "dib");
	struct session_track_data* tracks = NULL;
	int num_tracks = 0;
	load_session_tracks(&tracks, &num_tracks, user);
	char* content = render_resource(false, resource);
	strcpy(buffer->data, "{{ main }}");
	set_parameter(buffer, "main", mem_cache()->main_template);
	set_parameter(buffer, "user", user);
	render_session_tracks(buffer, "tracks", tracks, num_tracks);
	set_parameter(buffer, "content", content);
	free(content);
	free(tracks);
}

static void render_register(struct render_buffer* buffer) {
	strcpy(buffer->data, "{{ main }}");
	set_parameter(buffer, "main", mem_cache()->register_template);
}

static void render_login(struct render_buffer* buffer) {
	strcpy(buffer->data, "{{ main }}");
	set_parameter(buffer, "main", mem_cache()->login_template);
}

char* render_resource(bool is_main, const char* resource) {
	if (!resource) {
		return NULL;
	}
	struct render_buffer buffer;
	init_render_buffer(&buffer, 4096);
	if (is_main) {
		render_main(&buffer, resource);
		return buffer.data;
	}
	char page[64];
	resource = split_string(page, sizeof(page), resource, '/');
	if (!strcmp(page, "groups")) {
		struct group_thumb_data* thumbs = NULL;
		int num_thumbs = 0;
		load_group_thumbs(&thumbs, &num_thumbs);
		render_group_thumb_list(&buffer, thumbs, num_thumbs);
		free(thumbs);
	} else if (!strcmp(page, "group")) {
		char group_id_str[32];
		resource = split_string(group_id_str, sizeof(group_id_str), resource, '/');
		int id = atoi(group_id_str);
		if (id == 0) {
			return buffer.data;
		}
		struct group_data group;
		load_group(&group, id);
		if (strlen(group.name) > 0) {
			render_group(&buffer, &group);
		}
		free(group.tags);
		free(group.albums);
		free(group.tracks);
	} else if (!strcmp(page, "search")) {
		char type[32];
		char query[512];
		resource = split_string(type, sizeof(type), resource, '/');
		resource = split_string(query, sizeof(query), resource, '/');
		struct search_result_data* results = NULL;
		int num_results = 0;
		load_search_results(&results, &num_results, type, query);
		render_search_results(&buffer, results, num_results);
		free(results);
	} else if (!strcmp(page, "upload")) {
		struct upload_data upload;
		load_upload(&upload);
		render_upload(&buffer, &upload);
		free(upload.uploads);
	} else if (!strcmp(page, "prepare")) {
		struct prepare_data* prepare = (struct prepare_data*)malloc(sizeof(struct prepare_data));
		if (!prepare) {
			return buffer.data;
		}
		char prepare_directory[1024];
		resource = split_string(prepare_directory, sizeof(prepare_directory), resource, '/');
		load_prepare(prepare, prepare_directory);
		render_prepare(&buffer, prepare);
		for (int i = 0; i < prepare->num_attachments; i++) {
			free(prepare->attachments[i].targets.options);
		}
		free(prepare);
	} else if (!strcmp(page, "prepare_attachment")) {
		struct prepare_attachment_data attachment;
		memset(&attachment, 0, sizeof(attachment));
		char folder[512];
		char filename[512];
		resource = split_string(folder, 512, resource, '/');
		resource = split_string(filename, 512, resource, '/');
		char path[1060];
		sprintf(path, "%s/%s", folder, filename);
		load_prepare_attachment(&attachment, path);
		strcpy(buffer.data, "{{ attachment }}");
		render_prepare_attachments(&buffer, "attachment", &attachment, 1);
		free(attachment.targets.options);
	} else if (!strcmp(page, "session_track")) {
		struct session_track_data* tracks = NULL;
		int num_tracks = 0;
		load_session_tracks(&tracks, &num_tracks, "dib");
		if (num_tracks > 0) {
			strcpy(buffer.data, "{{ track }}");
			render_session_track(&buffer, "track", &tracks[num_tracks - 1]);
			free(tracks);
		}
	} else if (!strcmp(page, "playlists")) {
		
	} else if (!strcmp(page, "add_group")) {
		struct add_group_data add_group;
		load_add_group(&add_group);
		render_add_group(&buffer, &add_group);
	} else if (!strcmp(page, "profile")) {
		render_profile(&buffer);
	} else if (!strcmp(page, "register")) {
		render_register(&buffer);
	} else if (!strcmp(page, "login")) {
		render_login(&buffer);
	} else {
		fprintf(stderr, "Cannot render unknown page: %s\n", page);
	}
	return buffer.data;
}
