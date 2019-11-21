#include "config.h"
#include "files.h"
#include "http.h"
#include "route.h"
#include "stack.h"
#include "system.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

void route_form_upload(struct http_data* data) {
	int num_albums = http_data_parameter_array_size(data, "albums");
	if (num_albums > 20) {
		print_error("Too many albums at once. 20 is the limit.");
		return; // calm down.
	}
	for (int i = 0; i < num_albums; i++) {
		struct http_data_part* album = http_data_param(data, "albums", i);
		if (!strstr(album->filename, ".zip")) {
			print_error_f("Not a zip file: %s", album->filename);
			continue;
		}
		if (strchr(album->filename, '"')) {
			print_error_f("Invalid file name: %s", album->filename);
			continue;
		}
		const char* zip_path = push_string(server_uploaded_file_path(album->filename));
		char* unzip_filename = push_string_f("%lld%i %s", (long long)time(NULL), 1000 + rand() % 9000, album->filename);
		const char* unzip_directory = server_uploaded_file_path(unzip_filename);
		pop_string();
		char* unzip_dir_dot = strrchr(unzip_directory, '.');
		*unzip_dir_dot = '\0';
		print_info_f("Creating directory: %s", unzip_directory);
		if (create_directory(unzip_directory)) {
			print_info_f("Writing file: %s", zip_path);
			write_file(zip_path, album->value, album->size);
			system_execute(replace_temporary_string("unzip \"%s\" -d \"%s\"", zip_path, unzip_directory));
			pop_string();
		}
	}
}
