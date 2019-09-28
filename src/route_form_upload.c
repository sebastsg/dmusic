#include "route.h"
#include "http.h"
#include "config.h"
#include "files.h"
#include "system.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

void route_form_upload(struct http_data* data) {
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
			print_error_f("Invalid file name: %s", album->filename);
			continue;
		}
		const char* zip_path = server_uploaded_file_path(album->filename);
		char unzip_filename[1024];
		snprintf(unzip_filename, sizeof(unzip_filename), "%lld%i %s", (long long)time(NULL), 1000 + rand() % 9000, album->filename);
		const char* unzip_dir = server_uploaded_file_path(unzip_filename);
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
