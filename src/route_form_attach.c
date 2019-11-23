#include "analyze.h"
#include "config.h"
#include "files.h"
#include "format.h"
#include "http.h"
#include "render.h"
#include "route.h"
#include "system.h"

#include <stdlib.h>
#include <string.h>

void add_file_extension_if_absent(char* data, size_t data_size, char* path, size_t path_size) {
	if (!strchr(path, '.')) {
		const char* extension = guess_file_extension(data, data_size);
		if (extension && path_size > strlen(path) + strlen(extension) + 1) {
			strcat(path, ".");
			strcat(path, extension);
		}
	}
}

void route_form_attach(struct route_parameters* parameters) {
	struct http_data* data = &parameters->data;
	const char* method = http_data_string(data, "method");
	const char* folder = http_data_string(data, "folder");
	if (!strcmp(method, "url")) {
		const char* url = http_data_string(data, "file");
		char file_name[512];
		const char* url_path = strrchr(url, '/');
		if (url_path) {
			snprintf(file_name, sizeof(file_name), "%s", url_path + 1);
		} else {
			const char* file_ext = strrchr(url, '.');
			snprintf(file_name, sizeof(file_name), "file-%i%s", (int)time(NULL), file_ext ? file_ext : "");
		}
		replace_victims_with(file_name, "!@%^*~|\"&=?/\\#", '_');
		size_t file_size = 0;
		char* file_data = download_http_file(url, &file_size, "GET", NULL, NULL);
		add_file_extension_if_absent(file_data, file_size, file_name, sizeof(file_name));
		if (write_file(server_uploaded_directory_file_path(folder, file_name), file_data, file_size)) {
			char resource[1024];
			sprintf(resource, "import-attachment/%s/%s", folder, file_name);
			set_route_result_html(parameters->result, render_resource(resource, parameters->session));
		}
		free(file_data);
	} else if (!strcmp(method, "file")) {
		struct http_data_part* file = http_data_param(data, "file", 0);
		if (!file->filename) {
			print_error("Not a file.\n");
			return;
		}
		if (write_file(server_uploaded_directory_file_path(folder, file->filename), file->value, file->size)) {
			char resource[1024];
			sprintf(resource, "import-attachment/%s/%s", folder, file->filename);
			set_route_result_html(parameters->result, render_resource(resource, parameters->session));
		}
	} else {
		print_error_f("Invalid attach method: %s", method);
	}
}
