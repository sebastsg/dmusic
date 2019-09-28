#include "route.h"
#include "http.h"
#include "network.h"
#include "transcode.h"
#include "session.h"
#include "database.h"
#include "format.h"
#include "session_track.h"

#include <string.h>
#include <stdlib.h>

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
	} else if (!strcmp(form, "import")) {
		route_form_import(&data);
	} else if (!strcmp(form, "attach")) {
		route_form_attach(result, &data);
	} else if (!strcmp(form, "addsessiontrack")) {
		route_form_add_session_track(result, &data);
	} else if (!strcmp(form, "downloadremote")) {
		route_form_download_remote(&data);
	} else if (!strcmp(form, "clear-session-playlist")) {
		delete_session_tracks(get_session_username());
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
