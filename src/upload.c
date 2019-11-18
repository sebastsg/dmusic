#include "upload.h"
#include "system.h"
#include "render.h"
#include "cache.h"
#include "files.h"
#include "config.h"
#include "format.h"
#include "database.h"
#include "import.h"
#include "http.h"
#include "generic.h"

#include <string.h>
#include <stdlib.h>
#include <dirent.h>

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
	int status;
	int completion;
	char* name;
};

static struct remote_entry_state* parse_rutorrent_history_json(char* json, int* count) {
	struct remote_entry_state* entries = NULL;
	int allocated = 0;
	resize_array((void**)&entries, sizeof(struct remote_entry_state), &allocated, 50);
	if (!entries) {
		return NULL;
	}
	// todo: actual json parsing. not important for now, since it might not be needed more than this.
	const char* tag_action = "{\"action\":";
	const char* tag_name = "\"name\":\"";
	const char* tag_size = "\"size\":";
	const char* tag_downloaded = "\"downloaded\":";
	char* it = json;
	while (it = strstr(it, tag_action)) {
		char* action = it + strlen(tag_action);
		char* name = strstr(it, tag_name);
		char* size = strstr(it, tag_size);
		char* downloaded = strstr(it, tag_downloaded);
		if (!name || !size || !downloaded) {
			break;
		}
		name += strlen(tag_name);
		size += strlen(tag_size);
		downloaded += strlen(tag_downloaded);
		char* name_end = strstr(name, "\",\"");
		char* size_end = strchr(size, ',');
		char* downloaded_end = strchr(downloaded, ',');
		if (name_end > size || size_end > downloaded || !downloaded_end) {
			break;
		}
		*name_end = '\0';
		*size_end = '\0';
		*downloaded_end = '\0';
		resize_array((void**)&entries, sizeof(struct remote_entry_state), &allocated, *count + 1);
		struct remote_entry_state* entry = &entries[*count];
		entry->status = atoi(action);
		entry->completion = atoll(downloaded) * 100 / atoll(size);
		const size_t name_size = strlen(name) + 1;
		entry->name = (char*)malloc(name_size);
		if (entry->name) {
			strcpy(entry->name, name);
			json_decode_string(entry->name);
		}
		(*count)++;
		it = downloaded_end + 1;
	}
	return entries;
}

struct remote_entry_state* load_ftp_status(int* count) {
	char* status = download_http_file(get_property("ftp.status"), NULL);
	if (status) {
		// todo: different parsers
		struct remote_entry_state* entries = parse_rutorrent_history_json(status, count);
		free(status);
		return entries;
	} else {
		return NULL;
	}
}

void render_upload(struct render_buffer* buffer) {
	struct upload_data upload;
	load_upload(&upload);
	assign_buffer(buffer, get_cached_file("html/import_albums.html", NULL));
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
					if (entry->completion >= 100) {
						set_parameter(&entry_buffer, "status", "&#10004;");
					} else {
						char completion[32];
						snprintf(completion, sizeof(completion), "&#8595; %i%%", entry->completion);
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
	const char* path = server_uploaded_file_path(directory);
	char command[1024];
	snprintf(command, sizeof(command), "rm -R \"%s\"", path);
	print_info_f("Executing command: %s", command);
	system(command);
}
