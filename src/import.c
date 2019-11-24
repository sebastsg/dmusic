#if !defined(_XOPEN_SOURCE) || _XOPEN_SOURCE < 500
#define _XOPEN_SOURCE 500
#endif

#include "import.h"
#include "analyze.h"
#include "cache.h"
#include "config.h"
#include "format.h"
#include "generic.h"
#include "render.h"
#include "stack.h"
#include "system.h"

#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <stdlib.h>
#include <string.h>

static void* nftw_user_data_1 = NULL;
static void* nftw_user_data_2 = NULL;
static void* nftw_user_data_3 = NULL;

static void free_import_attachment(struct import_attachment_data* attachment) {
	free(attachment->targets.options);
	free(attachment->name);
	free(attachment->link);
	free(attachment->preview);
	free(attachment->path);
}

static void free_import_attachments(struct import_attachment_data** attachments, int count) {
	for (int i = 0; i < count; i++) {
		free_import_attachment(&(*attachments)[i]);
	}
	free(*attachments);
	*attachments = NULL;
}

static void free_import_discs(struct import_disc_data** discs) {
	free(*discs);
	*discs = NULL;
}

void load_import_attachment(struct import_attachment_data* attachment, const char* path) {
	const char* name = strrchr(path, '/') + 1;
	attachment->name = copy_string(name);
	attachment->path = copy_string(path);
	attachment->link = copy_string(client_uploaded_file_path(path));
	if (is_extension_image(path)) {
		attachment->preview = copy_string(attachment->link);
	} else {
		attachment->preview = NULL;
	}
	attachment->selected_target = guess_targets(name, &attachment->targets);
}

/*
 * @nftw_user_data_1: struct import_attachment_data** attachments
 * @nftw_user_data_2: int* num_attachments
 * @nftw_user_data_3: int* allocated_attachments
 */
static int nftw_guess_attachment(const char* path, const struct stat* stat_buffer, int flag, struct FTW* ftw_buffer) {
	if (flag != FTW_F) {
		return 0;
	}
	if (is_extension_audio(strrchr(path, '.'))) {
		return 0;
	}
	print_info_f("Attachment file: %s", path);
	struct import_attachment_data** attachments = (struct import_attachment_data**)nftw_user_data_1;
	int* num_attachments = (int*)nftw_user_data_2;
	int* allocated_attachments = (int*)nftw_user_data_3;
	resize_array((void**)attachments, sizeof(struct import_attachment_data), allocated_attachments, *num_attachments + 1);
	size_t uploads_path_size = strlen(get_property("path.uploads"));
	const char* relative_path = (strlen(path) > uploads_path_size ? path + uploads_path_size : path);
	load_import_attachment(&(*attachments)[*num_attachments], relative_path);
	(*num_attachments)++;
	return 0;
}

static void guess_attachments(struct import_attachment_data** attachments, int* num_attachments, int* allocated_attachments, const char* path) {
	*num_attachments = 0;
	const char* full_path = push_string(server_uploaded_file_path(path));
	nftw_user_data_1 = attachments;
	nftw_user_data_2 = num_attachments;
	nftw_user_data_3 = allocated_attachments;
	if (nftw(full_path, nftw_guess_attachment, 20, 0) == -1) {
		print_errno_f("Error occured while importing attachments: %s.", full_path);
	}
	pop_string();
}

/*
 * @nftw_user_data_1: struct import_disc_data** discs
 * @nftw_user_data_2: int* num_discs
 * @nftw_user_data_3: int* allocated_discs
 */
static int nftw_guess_track(const char* path, const struct stat* stat_buffer, int flag, struct FTW* ftw_buffer) {
	if (flag != FTW_F) {
		return 0;
	}
	if (!is_extension_audio(strrchr(path, '.'))) {
		return 0;
	}
	print_info_f("Track file: %s", path);
	struct import_disc_data** discs = (struct import_disc_data**)nftw_user_data_1;
	int* num_discs = (int*)nftw_user_data_2;
	int* allocated_discs = (int*)nftw_user_data_3;
	resize_array((void**)discs, sizeof(struct import_disc_data), allocated_discs, *num_discs + 1);
	size_t uploads_path_size = strlen(get_property("path.uploads"));
	const char* uploaded_name = strrchr(path, '/') + 1;
	int disc_num = guess_disc_num(uploaded_name);
	if (disc_num > 5) {
		// TODO: Compare with number of tracks? If there are 100 audio files, perhaps it is right.
		print_error_f("The filenames make it seem like there are %i discs. This is unlikely, and currently not allowed.", disc_num);
		return 0;
	}
	while (disc_num > *num_discs || *num_discs == 0) {
		(*discs)[*num_discs].num = *num_discs + 1;
		(*discs)[*num_discs].num_tracks = 0;
		(*num_discs)++;
	}
	struct import_disc_data* disc = &((*discs)[disc_num - 1]);
	struct import_track_data* track = &disc->tracks[disc->num_tracks];
	strcpy(disc->name, "");
	disc->num_tracks++;
	track->num = guess_track_num(uploaded_name, true);
	guess_track_name(track->fixed, uploaded_name);
	strcpy(track->uploaded, uploaded_name);
	strcpy(track->extension, strrchr(uploaded_name, '.') + 1);
	strcpy(track->path, path + uploads_path_size);
	return 0;
}

static int sort_import_track_num(const void* a, const void* b) {
	return ((struct import_track_data*)a)->num - ((struct import_track_data*)b)->num;
}

static void guess_tracks(struct import_disc_data** discs, int* num_discs, int* allocated_discs, const char* path) {
	*num_discs = 0;
	const char* full_path = push_string(server_uploaded_file_path(path));
	nftw_user_data_1 = discs;
	nftw_user_data_2 = num_discs;
	nftw_user_data_3 = allocated_discs;
	if (nftw(full_path, nftw_guess_track, 20, 0) == -1) {
		print_errno_f("Error occured while importing tracks: " A_CYAN "%s" A_RED ".", full_path);
	}
	pop_string();
	for (int i = 0; i < *num_discs; i++) {
		qsort((*discs)[i].tracks, (*discs)[i].num_tracks, sizeof(struct import_track_data), sort_import_track_num);
	}
}

bool find_upload_path_by_prefix(char* dest, const char* prefix) {
	*dest = '\0';
	const char* uploads = get_property("path.uploads");
	DIR* dir = opendir(uploads);
	if (!dir) {
		print_error_f("Failed to read uploads directory: " A_CYAN "%s", uploads);
		return false;
	}
	struct dirent* entry = NULL;
	while (entry = readdir(dir)) {
		if (strstr(entry->d_name, prefix)) {
			strcpy(dest, entry->d_name);
			break;
		}
	}
	closedir(dir);
	return *dest != '\0';
}

void load_import(struct import_data* import, const char* prefix) {
	memset(import, 0, sizeof(struct import_data));
	char path[256];
	if (!find_upload_path_by_prefix(path, prefix)) {
		print_error_f("Did not find upload path based on prefix " A_CYAN "%s", prefix);
		return;
	}
	strcpy(import->filename, path + strlen(prefix) + 1);
	guess_album_name(import->album_name, import->filename);
	sprintf(import->released_at, "%i-01-01", guess_album_year(import->filename)); // TODO: full date
	strcpy(import->folder, path);
	strcpy(import->group_search.name, "groups");
	strcpy(import->group_search.type, "groups");

	guess_group_name(import->group_search.value, import->group_search.text, import->filename);

	// Remove the group name in the album name:
	char* group_name_in_album = strstr(import->album_name, import->group_search.text);
	if (group_name_in_album) {
		int group_name_length = strlen(import->group_search.text);
		replace_substring(import->album_name, group_name_in_album, import->album_name + group_name_length, sizeof(import->album_name), "");
		trim_ends(import->album_name, " \t");
	}

	guess_tracks(&import->discs, &import->num_discs, &import->allocated_discs, path);
	guess_attachments(&import->attachments, &import->num_attachments, &import->allocated_attachments, path);
	guess_album_type(import->album_type, import->num_discs > 0 ? import->discs[0].num_tracks : 0);
	strcpy(import->album_release_type, "original"); // todo: guess
	guess_audio_format(import->audio_format, import->filename);
}

void render_import(struct render_buffer* buffer, const char* prefix) {
	struct import_data import;
	load_import(&import, prefix);
	if (import.num_discs < 1 || import.discs[0].num_tracks < 1) {
		const int num_tracks = (import.num_discs > 0 ? import.discs[0].num_tracks : 0);
		print_error_f("Invalid number of discs (%i) or tracks (%i)", import.num_discs, num_tracks);
		return;
	}
	strcpy(buffer->data, get_cached_file("html/import.html", NULL));
	set_parameter(buffer, "filename", import.filename);
	set_parameter(buffer, "name", import.album_name);
	set_parameter(buffer, "released_at", import.released_at);
	render_search(buffer, "group_search", &import.group_search);
	struct render_buffer select_buffer;
	init_render_buffer(&select_buffer, 2048);
	const char* extension = import.discs[0].tracks[0].extension;
	const struct select_options* audio_formats = get_cached_options("audio_format");
	for (int i = 0; i < audio_formats->count; i++) {
		struct select_option* format = &audio_formats->options[i];
		if (strstr(format->name, "FLAC") && strcmp(extension, "flac")) {
			continue;
		}
		if (strstr(format->name, "MP3") && strcmp(extension, "mp3")) {
			continue;
		}
		if (strstr(format->name, "AAC") && strcmp(extension, "m4a")) {
			continue;
		}
		append_buffer(&select_buffer, "{{ option }}");
		bool selected = !strcmp(import.audio_format, format->code);
		render_select_option(&select_buffer, "option", format->code, format->name, selected);
	}
	set_parameter(buffer, "formats", select_buffer.data);
	select_buffer.offset = 0;
	select_buffer.data[0] = '\0';
	const struct select_options* album_types = get_cached_options("album_type");
	for (int i = 0; i < album_types->count; i++) {
		struct select_option* type = &album_types->options[i];
		append_buffer(&select_buffer, "{{ option }}");
		bool selected = !strcmp(import.album_type, type->code);
		render_select_option(&select_buffer, "option", type->code, type->name, selected);
	}
	set_parameter(buffer, "types", select_buffer.data);
	select_buffer.offset = 0;
	select_buffer.data[0] = '\0';
	const struct select_options* album_release_types = get_cached_options("album_release_type");
	for (int i = 0; i < album_release_types->count; i++) {
		struct select_option* type = &album_release_types->options[i];
		append_buffer(&select_buffer, "{{ option }}");
		bool selected = !strcmp(import.album_release_type, type->code);
		render_select_option(&select_buffer, "option", type->code, type->name, selected);
	}
	set_parameter(buffer, "release_types", select_buffer.data);
	render_import_attachments(buffer, import.attachments, import.num_attachments);
	set_parameter(buffer, "folder", import.folder);
	struct render_buffer discs_buffer;
	init_render_buffer(&discs_buffer, 8192);
	for (int i = 0; i < import.num_discs; i++) {
		append_buffer(&discs_buffer, "{{ disc }}");
		render_import_disc(&discs_buffer, "disc", &import.discs[i]);
	}
	set_parameter(buffer, "discs", discs_buffer.data);
	free(discs_buffer.data);
	free(select_buffer.data);
	free_import_attachments(&import.attachments, import.num_attachments);
	free_import_discs(&import.discs);
}

void render_attachment(struct render_buffer* buffer, struct import_attachment_data* attachment) {
	append_buffer(buffer, get_cached_file("html/import_attachment.html", NULL));
	const char* extension = strrchr(attachment->name, '.');
	const char* blacklist[] = { ".sfv", ".m3u", ".nfo" };
	bool checked = true;
	for (int j = 0; extension && j < 3; j++) {
		if (!strcmp(extension, blacklist[j])) {
			checked = false;
			break;
		}
	}
	set_parameter(buffer, "checked", checked ? "checked" : "");
	set_parameter(buffer, "path", attachment->path);
	struct render_buffer select_buffer;
	init_render_buffer(&select_buffer, 2048);
	for (int j = 0; j < attachment->targets.count; j++) {
		struct select_option* target = &attachment->targets.options[j];
		append_buffer(&select_buffer, "{{ option }}");
		render_select_option(&select_buffer, "option", target->code, target->name, j == attachment->selected_target);
	}
	set_parameter(buffer, "targets", select_buffer.data);
	free(select_buffer.data);
	set_parameter(buffer, "link", attachment->link);
	set_parameter(buffer, "name", attachment->name);
	set_parameter(buffer, "link", attachment->link);
	set_parameter(buffer, "preview", attachment->preview);
}

void render_import_attachments(struct render_buffer* buffer, struct import_attachment_data* attachments, int num_attachments) {
	struct render_buffer attachments_buffer;
	init_render_buffer(&attachments_buffer, 4096);
	for (int i = 0; i < num_attachments; i++) {
		render_attachment(&attachments_buffer, &attachments[i]);
	}
	set_parameter(buffer, "attachments", attachments_buffer.data);
	free(attachments_buffer.data);
}

void render_import_attachment(struct render_buffer* buffer, const char* directory, const char* filename) {
	struct import_attachment_data attachment;
	memset(&attachment, 0, sizeof(attachment));
	char* path = push_string_f("%s/%s", directory, filename);
	load_import_attachment(&attachment, path);
	pop_string();
	assign_buffer(buffer, "{{ attachments }}");
	render_import_attachments(buffer, &attachment, 1);
	free_import_attachment(&attachment);
}

void render_import_disc(struct render_buffer* buffer, const char* key, struct import_disc_data* disc) {
	set_parameter(buffer, key, get_cached_file("html/import_disc.html", NULL));
	set_parameter_int(buffer, "num", disc->num);
	set_parameter(buffer, "name", disc->name);
	struct render_buffer tracks_buffer;
	init_render_buffer(&tracks_buffer, 4096);
	for (int i = 0; i < disc->num_tracks; i++) {
		// todo: fix potential security issue where a path has {{ fixed }} etc...
		append_buffer(&tracks_buffer, get_cached_file("html/import_track.html", NULL));
		set_parameter_int(&tracks_buffer, "num", disc->tracks[i].num);
		set_parameter(&tracks_buffer, "path", disc->tracks[i].path);
		set_parameter(&tracks_buffer, "fixed", disc->tracks[i].fixed);
		set_parameter(&tracks_buffer, "uploaded", disc->tracks[i].uploaded);
	}
	set_parameter(buffer, "tracks", tracks_buffer.data);
	free(tracks_buffer.data);
}
