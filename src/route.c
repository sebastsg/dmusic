#include "route.h"
#include "format.h"
#include "cache.h"
#include "config.h"
#include "http.h"
#include "network.h"
#include "database.h"
#include "files.h"
#include "transcode.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

void route_file(struct route_result* result, const char* path) {
	printf("Serving file: %s\n", path);
	size_t file_size = 0;
	result->body = read_file(path, &file_size);
	result->size = file_size;
	result->freeable = true;
	strcpy(result->type, http_file_content_type(path));
}

void route_uploaded_file(struct route_result* result, const char* resource) {
	char path[1024];
	route_file(result, server_uploaded_file_path(path, resource));
}

void route_render(struct route_result* result, const char* resource) {
	result->body = render_resource(false, resource);
	if (result->body) {
		strcpy(result->type, "text/html");
		result->size = strlen(result->body);
		result->freeable = true;
	}
}

void route_render_main(struct route_result* result, const char* resource) {
	if (strlen(resource) == 0) {
		resource = "playlists";
	}
	result->body = render_resource(true, resource);
	if (result->body) {
		strcpy(result->type, "text/html");
		result->size = strlen(result->body);
		result->freeable = true;
	}
}

void route_image(struct route_result* result, const char* resource) {
	if (!resource) {
		return;
	}
	char image[32];
	resource = split_string(image, sizeof(image), resource, '/');
	if (!strcmp(image, "album")) {
		char album_release_id_str[32];
		resource = split_string(album_release_id_str, 32, resource, '/');
		int album_release_id = atoi(album_release_id_str);
		char image_format[32];
		resource = split_string(image_format, 32, resource, '/');
		char image_path[512];
		int num = first_album_attachment_of_type(album_release_id, "cover");
		if (num > 0) {
			route_file(result, server_album_image_path(image_path, album_release_id, num));
		}
	} else if (!strcmp(image, "group")) {
		char group_id_str[32];
		resource = split_string(group_id_str, 32, resource, '/');
		int group_id = atoi(group_id_str);
		char image_format[32];
		resource = split_string(image_format, 32, resource, '/');
		char image_path[512];
		int num = first_group_attachment_of_type(group_id, "background");
		if (num > 0) {
			route_file(result, server_group_image_path(image_path, group_id, num));
		}
	} else if (!strcmp(image, "flag")) {

	} else if (!strcmp(image, "missing.png")) {

	} else if (!strcmp(image, "bg.jpg")) {
		result->body = mem_cache()->bg_jpg;
		result->size = mem_cache()->bg_jpg_size;
		strcpy(result->type, "image/jpeg");
	}
}

void route_track(struct route_result* result, const char* resource) {
	if (!resource) {
		return;
	}
	printf("Range: %s\n", result->client->headers.range);
	int album_release_id = 0;
	int disc_num = 0;
	int track_num = 0;
	int num = sscanf(resource, "%i/%i/%i", &album_release_id, &disc_num, &track_num);
	if (num != 3) {
		fprintf(stderr, "Invalid request: %s\n", resource);
		return;
	}
	char format[16];
	find_best_audio_format(format, album_release_id, false);
	char path[512];
	server_track_path(path, format, album_release_id, disc_num, track_num);
	route_file(result, path);
}

static void route_form_add_group(struct http_data* data) {
	const char* name = http_data_string(data, "name");
	const char* country = http_data_string(data, "country");
	const char* website = http_data_string(data, "website");
	const char* description = http_data_string(data, "description");
	const char* params[] = { country, name, website, description };
	int group_id = insert_row("group", true, 4, params);
	if (!group_id) {
		fputs("Failed to add group\n", stderr);
		return;
	}
	char group_id_str[32];
	sprintf(group_id_str, "%i", group_id);
	char directory[256];
	create_directory(group_path(directory, 256, group_id));
	// tags
	int priority = 1;
	char tag[128];
	const char* tags = http_data_string(data, "tags");
	while (tags = split_string(tag, 128, tags, ',')) {
		char priority_str[32];
		sprintf(priority_str, "%i", priority);
		const char* tag_params[] = { group_id_str, tag, priority_str };
		insert_row("group_tag", false, 3, tag_params);
		priority++;
	}
	// people
	int num_people = http_data_parameter_array_size(data, "person-id");
	if (num_people > 20) {
		return; // seems fishy.
	}
	for (int i = 0; i < num_people; i++) {
		const char* person_id = http_data_string_at(data, "person-id", i);
		const char* role = http_data_string_at(data, "person-role", i);
		const char* started_at = "";
		const char* ended_at = "";
		const char* person_params[] = { group_id_str, person_id, role, started_at, ended_at };
		insert_row("group_member", false, 5, person_params);
	}
	// images
	int num_images = http_data_parameter_array_size(data, "image-source");
	if (num_images > 50) {
		return; // at this point, images should be added afterwards.
	}
	for (int i = 0; i < num_images; i++) {
		const char* source = http_data_string_at(data, "image-source", i);
		const char* is_background = http_data_string_at(data, "image-is-background", i);
		struct http_data_part* image_file = http_data_param(data, "image-file", i);
		if (!image_file) {
			continue;
		}
		const char* image_description = http_data_string_at(data, "image-description", i);
		char dest_path[512];
		sprintf(dest_path, "%s/%i", directory, i + 1);
		const char* image_extension = strrchr(image_file->filename, '.');
		if (image_extension) {
			strcat(dest_path, image_extension);
		}
		if (!strcmp(source, "url")) {
			// todo: download from url
			fputs("Images from URL not supported yet.\n", stderr);
			continue;
		} else if (!strcmp(source, "file")) {
			write_file(dest_path, image_file->value, image_file->size);
		}
		char num_str[32];
		sprintf(num_str, "%i", i + 1);
		char attachment_type[16];
		strcpy(attachment_type, "background"); // todo: use is_background
		const char* image_params[] = { group_id_str, num_str, attachment_type, image_description };
		insert_row("group_attachment", false, 4, image_params);
	}
}

static void route_form_upload(struct http_data* data) {
	int num_albums = http_data_parameter_array_size(data, "albums");
	if (num_albums > 20) {
		return; // calm down.
	}
	for (int i = 0; i < num_albums; i++) {
		struct http_data_part* album = http_data_param(data, "albums", i);
		if (!strstr(album->filename, ".zip")) {
			continue;
		}
		if (strchr(album->filename, '"')) {
			fprintf(stderr, "Invalid file name: %s\n", album->filename);
			continue;
		}
		char zip_path[1024];
		server_uploaded_file_path(zip_path, album->filename);
		char unzip_filename[512];
		sprintf(unzip_filename, "%lld%i %s", (long long)time(NULL), 1000 + rand() % 9000, album->filename);
		char unzip_dir[1024];
		server_uploaded_file_path(unzip_dir, unzip_filename);
		char* unzip_dir_dot = strrchr(unzip_dir, '.');
		*unzip_dir_dot = '\0';
		if (!create_directory(unzip_dir)) {
			continue;
		}
		write_file(zip_path, album->value, album->size);
		char unzip_cmd[4096];
		sprintf(unzip_cmd, "unzip \"%s\" -d \"%s\"", zip_path, unzip_dir);
		puts(unzip_cmd);
		system(unzip_cmd);
	}
}

static void route_form_prepare(struct http_data* data) {
	int num_discs = http_data_parameter_array_size(data, "disc-num");
	if (num_discs > 10) {
		return; // doesn't seem right.
	}
	const char* name = http_data_string(data, "name");
	const char* type = http_data_string(data, "type");
	const char* album_params[] = { name, type };
	int album_id = insert_row("album", true, 2, album_params);
	if (album_id == 0) {
		fprintf(stderr, "Failed to add album: %s/%s\n", name, type);
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
		fprintf(stderr, "Failed to add album release for %i\n", album_id);
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
	char album_dir[512];
	album_path(album_dir, 512, album_release_id);
	create_directory(album_dir);
	int current_track = 0;
	int num_tracks = http_data_parameter_array_size(data, "track-num");
	for (int i = 0; i < num_discs; i++) {
		const char* disc_num = http_data_string_at(data, "disc-num", i);
		const char* disc_name = http_data_string_at(data, "disc-name", i);
		const char* disc_params[] = { album_release_id_str, disc_num, disc_name };
		insert_row("disc", false, 3, disc_params);
		char disc_dir[512];
		char disc_format_dir[512];
		server_disc_path(disc_dir, album_release_id, atoi(disc_num));
		server_disc_format_path(disc_format_dir, album_release_id, atoi(disc_num), format);
		create_directory(disc_dir);
		create_directory(disc_format_dir);
		int disc_tracks = atoi(http_data_string_at(data, "disc-tracks", i));
		disc_tracks += current_track;
		while (disc_tracks > current_track && current_track < num_tracks) {
			const char* track_num = http_data_string_at(data, "track-num", current_track);
			const char* track_name = http_data_string_at(data, "track-name", current_track);
			const char* track_path = http_data_string_at(data, "track-path", current_track);
			char track_src_path[512];
			char track_dest_path[512];
			server_uploaded_file_path(track_src_path, track_path);
			server_track_path(track_dest_path, format, album_release_id, atoi(disc_num), atoi(track_num));
			char copy_command[1200];
			sprintf(copy_command, "cp -p \"%s\" \"%s\"", track_src_path, track_dest_path);
			printf("Executing command: %s\n", copy_command);
			system(copy_command);
			char soxi_command[1024];
			sprintf(soxi_command, "soxi -D \"%s\"", track_dest_path);
			printf("Executing command: %s\n", soxi_command);
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
		char attachment_src_path[512];
		server_uploaded_file_path(attachment_src_path, attachment_path);
		char attachment_dest_path[600];
		const char* extension = strrchr(attachment_path, '.');
		sprintf(attachment_dest_path, "%s/%i%s", album_dir, i + 1, extension ? extension : "");
		char copy_command[1200];
		sprintf(copy_command, "cp -p \"%s\" \"%s\"", attachment_src_path, attachment_dest_path);
		printf("Executing command: %s\n", copy_command);
		system(copy_command);
	}
	if (!strncmp(format, "flac", 4)) {
		transcode_album_release(album_release_id, "mp3-320");
	}
}

static void route_form_attach(struct route_result* result, struct http_data* data) {
	const char* method = http_data_string(data, "method");
	const char* folder = http_data_string(data, "folder");
	if (!strcmp(method, "url")) {
		const char* url = http_data_string(data, "file");
		char file_name[512];
		const char* url_base = strrchr(url, '/');
		if (url_base) {
			snprintf(file_name, sizeof(file_name), "%s", url_base + 1);
		} else {
			const char* file_ext = strrchr(url, '.');
			snprintf(file_name, sizeof(file_name), "file-%i%s", (int)time(NULL), file_ext ? file_ext : "");
		}
		char dest_path[1024];
		server_uploaded_file_path(dest_path, folder);
		strcat(dest_path, "/");
		strcat(dest_path, file_name);
		size_t file_size = 0;
		char* file_data = download_http_file(url, &file_size);
		if (write_file(dest_path, file_data, file_size)) {
			char resource[1024];
			sprintf(resource, "prepare_attachment/%s/%s", folder, file_name);
			route_render(result, resource);
		}
	} else if (!strcmp(method, "file")) {
		struct http_data_part* file = http_data_param(data, "file", 0);
		if (!file->filename) {
			print_error("Not a file.\n");
			return;
		}
		char dest_path[1024];
		server_uploaded_file_path(dest_path, folder);
		strcat(dest_path, "/");
		strcat(dest_path, file->filename);
		if (write_file(dest_path, file->value, file->size)) {
			char resource[1024];
			sprintf(resource, "prepare_attachment/%s/%s", folder, file->filename);
			route_render(result, resource);
		}
	} else {
		fprintf(stderr, "Invalid attach method: %s\n", method);
	}
}

static void route_form_add_session_track(struct route_result* result, struct http_data* data) {
	int album_release_id = atoi(http_data_string(data, "album"));
	if (album_release_id == 0) {
		fputs("Album release must be specified when adding a session track", stderr);
		return;
	}
	int disc_num = atoi(http_data_string(data, "disc"));
	int track_num = atoi(http_data_string(data, "track"));
	int from_num = 0;
	int to_num = 0;
	if (track_num > 0) {
		from_num = create_session_track("dib", album_release_id, disc_num, track_num);
	} else if (disc_num > 0) {
		struct track_data* tracks = NULL;
		int count = 0;
		load_disc_tracks(&tracks, &count, album_release_id, disc_num);
		for (int i = 0; i < count; i++) {
			to_num = create_session_track("dib", album_release_id, disc_num, tracks[i].num);
			if (from_num == 0) {
				from_num = to_num;
			}
		}
	} else {
		struct track_data* tracks = NULL;
		int count = 0;
		load_album_tracks(&tracks, &count, album_release_id);
		for (int i = 0; i < count; i++) {
			to_num = create_session_track("dib", album_release_id, tracks[i].disc_num, tracks[i].num);
			if (from_num == 0) {
				from_num = to_num;
			}
		}
		free(tracks);
	}
	if (from_num == 0) {
		fputs("Failed to add session track.", stderr);
		return;
	}
	char resource[128];
	sprintf(resource, "session_tracks/%i/%i", from_num, to_num);
	route_render(result, resource);
}

static void route_form_download_remote(struct http_data* data) {
	const char* directory = http_data_string(data, "directory");
	if (strlen(directory) == 0) {
		return;
	}
	const char* user = get_property("ftp.user");
	const char* host = get_property("ftp.host");
	const char* root_dir = get_property("path.root");
	const char* uploads_dir = get_property("path.uploads");
	char dir[1024];
	strcpy(dir, directory);
	trim_ends(dir, " \t\r\n");
	char command[4096];
	sprintf(command, "%s/get_ftp.sh %s %s \"%s\" \"%s\"", root_dir, user, host, uploads_dir, dir);
	char* result = system_output(command, NULL);
	if (!result) {
		return;
	}
	puts(result);
	free(result);
	sprintf(command, "mv \"%s/%s\" \"%s/%lld%i %s\"", uploads_dir, dir, uploads_dir, (long long)time(NULL), 1000 + rand() % 9000, dir);
	puts(command);
	system(command);
}

void route_form(struct route_result* result, const char* resource, char* body, size_t size) {
	if (!resource) {
		return;
	}
	char form[32];
	resource = split_string(form, sizeof(form), resource, '/');
	struct http_data data = http_load_data(body, size, result->client->headers.content_type);
	if (!strcmp(form, "addgroup")) {
		route_form_add_group(&data);
	} else if (!strcmp(form, "upload")) {
		route_form_upload(&data);
	} else if (!strcmp(form, "prepare")) {
		route_form_prepare(&data);
	} else if (!strcmp(form, "attach")) {
		route_form_attach(result, &data);
	} else if (!strcmp(form, "addsessiontrack")) {
		route_form_add_session_track(result, &data);
	} else if (!strcmp(form, "downloadremote")) {
		route_form_download_remote(&data);
	} else if (!strcmp(form, "clear-session-playlist")) {
		delete_session_tracks("dib");
	} else if (!strcmp(form, "transcode")) {
		char album_release_id[32];
		char format[32];
		resource = split_string(album_release_id, 32, resource, '/');
		resource = split_string(format, 32, resource, '/');
		if (strlen(format) == 0) {
			strcpy(format, "mp3-320");
		}
		transcode_album_release(atoi(album_release_id), format);
	}
	http_free_data(&data);
}

void process_route(struct route_result* result, const char* resource, char* body, size_t size) {
	char command[32];
	const char* it = split_string(command, sizeof(command), resource, '/');
	if (!strcmp(command, "img")) {
		route_image(result, it);
		return;
	}
	if (!strcmp(command, "track")) {
		route_track(result, it);
		return;
	}
	if (!strcmp(command, "render")) {
		route_render(result, it);
		return;
	}
	if (!strcmp(command, "form")) {
		route_form(result, it, body, size);
		return;
	}
	if (!strcmp(command, "uploaded")) {
		route_uploaded_file(result, it);
		return;
	}
	if (!strcmp(command, "main.js")) {
		result->body = mem_cache()->main_js;
		result->size = strlen(result->body);
		strcpy(result->type, "application/javascript");
		return;
	}
	if (!strcmp(command, "style.css")) {
		result->body = mem_cache()->style_css;
		result->size = strlen(result->body);
		strcpy(result->type, "text/css");
		return;
	}
	if (!strcmp(command, "icon.png")) {
		result->body = mem_cache()->icon_png;
		result->size = mem_cache()->icon_png_size;
		strcpy(result->type, "image/png");
		return;
	}
	route_render_main(result, resource);
}

void free_route(struct route_result* result) {
	if (result->freeable) {
		free(result->body);
		result->freeable = false;
	}
	result->body = NULL;
	result->size = 0;
}
