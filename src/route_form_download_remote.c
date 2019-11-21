#include "config.h"
#include "format.h"
#include "http.h"
#include "route.h"
#include "stack.h"
#include "system.h"

#include <stdlib.h>
#include <string.h>

void route_form_download_remote(struct http_data* data) {
	const char* entry = http_data_string(data, "name");
	if (strlen(entry) == 0) {
		return;
	}
	const char* user = get_property("ftp.user");
	const char* host = get_property("ftp.host");
	const char* root_directory = get_property("path.root");
	const char* uploads_directory = get_property("path.uploads");
	char* remote_entry = push_string(entry);
	trim_ends(remote_entry, " \t\r\n");
	string_replace(remote_entry, size_of_top_string(), "$", "\\$");
	char* result = system_output(replace_temporary_string("%s/get_ftp.sh %s %s \"%s\" \"%s\"", root_directory, user, host, uploads_directory, remote_entry), NULL);
	if (result) {
		print_info_f(A_GREEN "%s", result);
		free(result);
		system_execute(replace_temporary_string("mv \"%s/%s\" \"%s/%lld%i %s\"", uploads_directory, remote_entry, uploads_directory, (long long)time(NULL), 1000 + rand() % 9000, remote_entry));
	}
	pop_string();
}
