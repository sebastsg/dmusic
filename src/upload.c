#include "upload.h"
#include "system.h"
#include "render.h"
#include "cache.h"
#include "files.h"
#include "config.h"
#include "format.h"
#include "database.h"
#include "import.h"

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
	render_sftp_ls(buffer);
	free(uploads_buffer.data);
	free(upload.uploads);
}

void render_sftp_ls(struct render_buffer* buffer) {
	char ftp_command[2048];
	sprintf(ftp_command, "%s/ls_ftp.sh %s %s", get_property("path.root"), get_property("ftp.user"), get_property("ftp.host"));
	char* output = system_output(ftp_command, NULL);
	if (!output) {
		return;
	}
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
				append_buffer(&entry_buffer, get_cached_file("html/download_remote_entry.html", NULL));
				set_parameter(&entry_buffer, "name", line);
			}
		}
	}
	PQclear(result);
	set_parameter(buffer, "remote-entries", entry_buffer.data);
	free(entry_buffer.data);
	free(output);
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
