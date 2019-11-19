#include "route.h"
#include "http.h"
#include "database.h"
#include "stack.h"
#include "system.h"
#include "config.h"
#include "transcode.h"
#include "files.h"

#include <string.h>
#include <stdlib.h>

void route_form_import(struct http_data* data) {
	int num_discs = http_data_parameter_array_size(data, "disc-num");
	if (num_discs > 10) {
		return; // doesn't seem right.
	}
	const char* name = http_data_string(data, "name");
	const char* type = http_data_string(data, "type");
	const char* album_params[] = { name, type };
	int album_id = insert_row("album", true, 2, album_params);
	if (album_id == 0) {
		print_error_f("Failed to add album: %s/%s", name, type);
		return;
	}
	const char* released_at = http_data_string(data, "released-at");
	const char* catalog = http_data_string(data, "catalog");
	const char* format = http_data_string(data, "format");
	const char* release_type = http_data_string(data, "release-type");
	char album_id_str[32];
	sprintf(album_id_str, "%i", album_id);
	const char* album_release_params[] = { album_id_str, release_type, catalog, released_at };
	int album_release_id = insert_row("album_release", true, 4, album_release_params);
	if (album_release_id == 0) {
		print_error_f("Failed to add album release for %i", album_id);
		return;
	}
	char album_release_id_str[32];
	sprintf(album_release_id_str, "%i", album_release_id);
	const char* params[] = { album_release_id_str, format };
	insert_row("album_release_format", false, 2, params);
	int num_groups = http_data_parameter_array_size(data, "groups");
	for (int i = 0; i < num_groups; i++) {
		const char* group_id_str = http_data_string_at(data, "groups", i);
		char priority_str[32];
		sprintf(priority_str, "%i", i + 1);
		const char* album_release_group_params[] = { album_release_id_str, group_id_str, priority_str };
		insert_row("album_release_group", false, 3, album_release_group_params);
	}
	const char* album_dir = push_string(server_album_path(album_release_id));
	create_directory(album_dir);
	int current_track = 0;
	int num_tracks = http_data_parameter_array_size(data, "track-num");
	for (int i = 0; i < num_discs; i++) {
		const char* disc_num = http_data_string_at(data, "disc-num", i);
		const char* disc_name = http_data_string_at(data, "disc-name", i);
		const char* disc_params[] = { album_release_id_str, disc_num, disc_name };
		insert_row("disc", false, 3, disc_params);
		create_directory(server_disc_path(album_release_id, atoi(disc_num)));
		create_directory(server_disc_format_path(album_release_id, atoi(disc_num), format));
		int disc_tracks = atoi(http_data_string_at(data, "disc-tracks", i));
		disc_tracks += current_track;
		while (disc_tracks > current_track && current_track < num_tracks) {
			const char* track_num = http_data_string_at(data, "track-num", current_track);
			const char* track_name = http_data_string_at(data, "track-name", current_track);
			const char* track_path = http_data_string_at(data, "track-path", current_track);
			const char* track_src_path = push_string(server_uploaded_file_path(track_path));
			const char* track_dest_path = server_track_path(format, album_release_id, atoi(disc_num), atoi(track_num));
			char copy_command[2048];
			snprintf(copy_command, 2048, "cp -p \"%s\" \"%s\"", track_src_path, track_dest_path);
			pop_string();
			print_info_f("%s", copy_command);
			system(copy_command);
			char soxi_command[1024];
			sprintf(soxi_command, "soxi -D \"%s\"", track_dest_path);
			print_info_f("%s", soxi_command);
			char* soxi_out = system_output(soxi_command, NULL);
			char seconds[32]; // in case of error, this will just be 0
			sprintf(seconds, "%i", atoi(soxi_out));
			const char* track_params[] = { album_release_id_str, disc_num, track_num, seconds, track_name };
			insert_row("track", false, 5, track_params);
			free(soxi_out);
			current_track++;
		}
	}
	int num_attachments = http_data_parameter_array_size(data, "attachment-path");
	for (int i = 0; i < num_attachments; i++) {
		char attachment_num[32];
		sprintf(attachment_num, "%i", i + 1);
		const char* attachment_type = http_data_string_at(data, "attachment-type", i);
		const char* attachment_path = http_data_string_at(data, "attachment-path", i);
		const char* description = "";
		const char* attachment_params[] = { album_release_id_str, attachment_num, attachment_type, description };
		insert_row("album_attachment", false, 4, attachment_params);
		const char* attachment_src_path = server_uploaded_file_path(attachment_path);
		char attachment_dest_path[600];
		const char* extension = strrchr(attachment_path, '.');
		sprintf(attachment_dest_path, "%s/%i%s", album_dir, i + 1, extension ? extension : "");
		char copy_command[1200];
		sprintf(copy_command, "cp -p \"%s\" \"%s\"", attachment_src_path, attachment_dest_path);
		print_info_f("%s", copy_command);
		system(copy_command);
	}
	pop_string();
	if (!strncmp(format, "flac", 4)) {
		transcode_album_release_async(album_release_id, "mp3-320");
	}
}
