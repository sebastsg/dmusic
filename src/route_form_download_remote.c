#include "route.h"
#include "system.h"
#include "http.h"
#include "config.h"
#include "format.h"

#include <stdlib.h>
#include <string.h>

void route_form_download_remote(struct http_data* data) {
	const char* entry = http_data_string(data, "name");
	if (strlen(entry) == 0) {
		return;
	}
	const char* user = get_property("ftp.user");
	const char* host = get_property("ftp.host");
	const char* root_dir = get_property("path.root");
	const char* uploads_dir = get_property("path.uploads");
	char dir[1024];
	strcpy(dir, entry);
	trim_ends(dir, " \t\r\n");
	string_replace(dir, sizeof(dir), "$", "\\$");
	char command[4096];
	sprintf(command, "%s/get_ftp.sh %s %s \"%s\" \"%s\"", root_dir, user, host, uploads_dir, dir);
	char* result = system_output(command, NULL);
	if (!result) {
		return;
	}
	print_info_f(A_GREEN "%s", result);
	free(result);
	sprintf(command, "mv \"%s/%s\" \"%s/%lld%i %s\"", uploads_dir, dir, uploads_dir, (long long)time(NULL), 1000 + rand() % 9000, dir);
	system_execute(command);
}
