#include "route.h"
#include "http.h"
#include "files.h"
#include "system.h"
#include "config.h"
#include "format.h"
#include "render.h"

#include <string.h>
#include <stdlib.h>

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
		char* file_data = download_http_file(url, &file_size);
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
