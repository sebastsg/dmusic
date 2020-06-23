#include "upload.h"
#include "cache.h"
#include "config.h"
#include "database.h"
#include "files.h"
#include "format.h"
#include "generic.h"
#include "http.h"
#include "import.h"
#include "render.h"
#include "stack.h"
#include "system.h"
#include "threads.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>

static int sort_uploaded_albums(const void* a, const void* b) {
	long long prefix_a = 0;
	long long prefix_b = 0;
	sscanf(((struct upload_uploaded_data*)a)->prefix, "%lld", &prefix_a);
	sscanf(((struct upload_uploaded_data*)b)->prefix, "%lld", &prefix_b);
	long long diff = prefix_a - prefix_b;
	return diff > 0 ? -1 : (diff < 0 ? 1 : 0);
}

void load_upload(struct upload_data* upload) {
	int allocated = 100;
	upload->num_uploads = 0;
	upload->uploads = (struct upload_uploaded_data*)malloc(allocated * sizeof(struct upload_uploaded_data));
	if (!upload->uploads) {
		return;
	}
	memset(upload->uploads, 0, allocated * sizeof(struct upload_uploaded_data));
	const char* uploads = get_property("path.uploads");
	DIR* dir = opendir(uploads);
	if (!dir) {
		print_error_f("Failed to read uploads directory: " A_CYAN "%s", uploads);
		return;
	}
	struct dirent* entry = NULL;
	while (entry = readdir(dir)) {
		if (!is_dirent_directory(uploads, entry)) {
			continue;
		}
		char* space = strchr(entry->d_name, ' ');
		if (!space) {
			continue;
		}
		struct upload_uploaded_data* uploaded = &upload->uploads[upload->num_uploads];
		strncpy(uploaded->prefix, entry->d_name, space - entry->d_name);
		strcpy(uploaded->name, space + 1);
		upload->num_uploads++;
		if (upload->num_uploads >= allocated) {
			break;
		}
	}
	closedir(dir);
	qsort(upload->uploads, upload->num_uploads, sizeof(struct upload_uploaded_data), sort_uploaded_albums);
}

struct remote_entry_state {
	int downloaded;
	int total;
	char* name;
};

static char* end_of_json_string(char* json) {
	if (json && *json == '"') {
		while (json = strchr(json + 1, '"')) {
			if (*(json - 1) != '\\') {
				return json;
			}
		}
	}
	return NULL;
}

static char* read_rutorrent_json_entry(char* json, struct remote_entry_state* entry) {
	// todo: actual json parsing. not important for now, since it might not be needed more than this.
	// key: hash
	//  0: is open,       1: is checking hash,      2: is hash checked,  3: state,         4: name
	//  5: size,          6: completed chunks,      7: total chunks,     8: downloaded,    9: uploaded
	// 10: ratio,        11: uplod limit,          12: download limit,  13: chunk size,   14: label
	// 15: peers actual, 16: peers not connected,  17: peers connected, 18: seeds actual, 19: remaining
	// 20: priority,     21: state changed (time), 22: skip total,      23: hashing (?),  24: hashed chunks
	// 25: path,         26: created (time),       27: tracker focus,   28: is active,    29: message
	// 30: comment,      31: free disk space,      32: is private,      33: multifile,    34: ?
	// 35: ?             36: ?                     37: ?                38: ?             39: ?, 40: ? (time), 41: ? (time)
	json += 44; //skip hash and ":[
	char* name = json + 16;
	if (*name != '"') {
		return NULL;
	}
	char* name_end = end_of_json_string(name);
	if (!name_end) {
		return NULL;
	}
	name++;
	char* completed = strchr(name_end + 3, '"');
	if (!completed) {
		return NULL;
	}
	completed += 3;
	char* completed_end = strchr(completed, '"');
	if (!completed_end) {
		return NULL;
	}
	char* total = completed_end + 3;
	char* total_end = strchr(total, '"');
	if (!total_end) {
		return NULL;
	}
	*name_end = '\0';
	*completed_end = '\0';
	*total_end = '\0';
	entry->downloaded = atoi(completed);
	entry->total = atoi(total);
	entry->name = (char*)malloc(strlen(name) + 1);
	if (entry->name) {
		strcpy(entry->name, name);
		json_decode_string(entry->name);
	}
	char* end = total_end;
	for (int i = 0; i < 34 && end; i++) {
		end = end_of_json_string(end + 2);
	}
	return end ? end + 3 : NULL;
}

static struct remote_entry_state* parse_rutorrent_json(char* json, int* count) {
	struct remote_entry_state* entries = NULL;
	int allocated = 0;
	resize_array((void**)&entries, sizeof(struct remote_entry_state), &allocated, 50);
	if (entries) {
		char* it = json + 6;
		while (it && *it == '"') {
			resize_array((void**)&entries, sizeof(struct remote_entry_state), &allocated, *count + 1);
			it = read_rutorrent_json_entry(it, &entries[*count]);
			if (it) {
				(*count)++;
			}
		}
	}
	return entries;
}

struct remote_entry_state* load_ftp_status(int* count) {
	const char* method = get_property("ftp.status.method");
	const char* url = get_property("ftp.status.url");
	const char* data = get_property("ftp.status.data");
	const char* type = get_property("ftp.status.type");
	char* status = download_http_file(url, NULL, method, data, type);
	if (status) {
		// todo: different parsers
		struct remote_entry_state* entries = parse_rutorrent_json(status, count);
		free(status);
		return entries;
	} else {
		return NULL;
	}
}

static size_t get_file_stat(const char* format, const char* path, const char* extension) {
	char* out = system_output(replace_temporary_string(format, path, extension), NULL);
	const size_t size = out ? atoll(out) : 0;
	free(out);
	return size;
}

static struct file_stats stats;

void get_stats_thread(void* data) {
	const char* groups = get_property("path.groups");
	const char* albums = get_property("path.albums");
	const char* uploads = get_property("path.uploads");
	const char* format = "find %s -type f -name \"*.%s\" -printf \"%%s\\n\" | gawk -M '{t+=$1}END{print t}'";
	stats.group_jpg = get_file_stat(format, groups, "jpg") / 1024 / 1024;
	stats.group_png = get_file_stat(format, groups, "png") / 1024 / 1024;
	stats.album_jpg = get_file_stat(format, albums, "jpg") / 1024 / 1024;
	stats.album_png = get_file_stat(format, albums, "png") / 1024 / 1024;
	stats.album_flac = get_file_stat(format, albums, "flac") / 1024 / 1024;
	stats.album_mp3 = get_file_stat(format, albums, "mp3") / 1024 / 1024;
	stats.uploads_total = get_file_stat(format, uploads, "jpg");
	stats.uploads_total += get_file_stat(format, uploads, "png");
	stats.uploads_total += get_file_stat(format, uploads, "flac");
	stats.uploads_total += get_file_stat(format, uploads, "mp3");
	stats.uploads_total /= 1024;
	stats.uploads_total /= 1024;
}

void load_stats_async() {
	open_thread(get_stats_thread, NULL);
}

void render_upload(struct render_buffer* buffer) {
	struct upload_data upload;
	load_upload(&upload);
	assign_buffer(buffer, get_cached_file("html/import_albums.html", NULL));
	append_buffer(buffer, "<br>Groups: {{ g }}<br>Album images: {{ au }}<br>Album FLAC: {{ af }}<br>Album MP3: {{ am }}<br>Uploads: {{ u }}");
	struct render_buffer uploads_buffer;
	init_render_buffer(&uploads_buffer, 2048);
	for (int i = 0; i < upload.num_uploads; i++) {
		append_buffer(&uploads_buffer, get_cached_file("html/import_album.html", NULL));
		set_parameter(&uploads_buffer, "prefix", upload.uploads[i].prefix);
		set_parameter(&uploads_buffer, "name", upload.uploads[i].name);
		set_parameter(&uploads_buffer, "prefix", upload.uploads[i].prefix);
	}
	set_parameter(buffer, "uploads", uploads_buffer.data);
	free(uploads_buffer.data);
	free(upload.uploads);
	set_parameter_int(buffer, "g", stats.group_jpg + stats.group_png);
	set_parameter_int(buffer, "au", stats.album_jpg + stats.album_png);
	set_parameter_int(buffer, "af", stats.album_flac);
	set_parameter_int(buffer, "am", stats.album_mp3);
	set_parameter_int(buffer, "u", stats.uploads_total);
}

void render_remote_entries(struct render_buffer* buffer) {
	render_sftp_ls(buffer);
}

struct remote_entry_state* find_remote_entry_state(struct remote_entry_state* entries, int count, const char* name) {
	for (int i = 0; i < count; i++) {
		if (!strcmp(entries[i].name, name)) {
			return &entries[i];
		}
	}
	return NULL;
}

void render_sftp_ls(struct render_buffer* buffer) {
	char ftp_command[2048];
	sprintf(ftp_command, "%s/ls_ftp.sh %s %s", get_property("path.root"), get_property("ftp.user"), get_property("ftp.host"));
	char* output = system_output(ftp_command, NULL);
	if (!output) {
		return;
	}
	int num_entries = 0;
	struct remote_entry_state* entries = load_ftp_status(&num_entries);
	struct render_buffer entry_buffer;
	init_render_buffer(&entry_buffer, 2048);
	char line[1024];
	const char* it = output;
	PGresult* result = execute_sql("select \"name\" from \"hidden_remote_entry\"", NULL, 0);
	const int hidden_count = PQntuples(result);
	while (it = split_string(line, 1024, it, '\n')) {
		if (!strchr(line, '>')) {
			trim_ends(line, " \t\r\n");
			bool hidden = false;
			for (int hidden_index = 0; hidden_index < hidden_count; hidden_index++) {
				if (!strcmp(line, PQgetvalue(result, hidden_index, 0))) {
					hidden = true;
					break;
				}
			}
			if (!hidden) {
				struct remote_entry_state* entry = find_remote_entry_state(entries, num_entries, line);
				append_buffer(&entry_buffer, get_cached_file("html/download_remote_entry.html", NULL));
				set_parameter(&entry_buffer, "name", line);
				if (entry) {
					if (entry->downloaded >= entry->total) {
						set_parameter(&entry_buffer, "status", "&#10004;");
					} else {
						char completion[32];
						const int percent = (int)((long long)entry->downloaded * 100ll / (long long)entry->total);
						snprintf(completion, sizeof(completion), "&#8595; %i%%", percent);
						set_parameter(&entry_buffer, "status", completion);
					}
				} else {
					set_parameter(&entry_buffer, "status", "?");
				}
				set_parameter(&entry_buffer, "name", line); // data for buttons
			}
		}
	}
	PQclear(result);
	append_buffer(buffer, entry_buffer.data);
	free(entry_buffer.data);
	free(output);
	for (int i = 0; i < num_entries; i++) {
		free(entries[i].name);
	}
	free(entries);
}

void delete_upload(const char* prefix) {
	if (strlen(prefix) == 0) {
		return;
	}
	char directory[512];
	if (!find_upload_path_by_prefix(directory, prefix)) {
		print_error_f("Did not find upload path based on prefix " A_CYAN "%s", prefix);
		return;
	}
	const char* path = push_string(server_uploaded_file_path(directory));
	system_execute(replace_temporary_string("rm -R \"%s\"", path));
	pop_string();
}
