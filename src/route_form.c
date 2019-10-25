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

void route_form_with_session(const char* form, struct route_parameters* parameters) {
	if (!strcmp(form, "addgroup")) {
		route_form_add_group(&parameters->data);
	} else if (!strcmp(form, "upload")) {
		route_form_upload(&parameters->data);
	} else if (!strcmp(form, "import")) {
		route_form_import(&parameters->data);
	} else if (!strcmp(form, "attach")) {
		route_form_attach(parameters);
	} else if (!strcmp(form, "addsessiontrack")) {
		route_form_add_session_track(parameters);
	} else if (!strcmp(form, "downloadremote")) {
		route_form_download_remote(&parameters->data);
	} else if (!strcmp(form, "clear-session-playlist")) {
		delete_session_tracks(parameters->session->name);
	} else if (!strcmp(form, "transcode")) {
		char album_release_id[32];
		char format[32];
		parameters->resource = split_string(album_release_id, 32, parameters->resource, '/');
		parameters->resource = split_string(format, 32, parameters->resource, '/');
		if (strlen(format) == 0) {
			strcpy(format, "mp3-320");
		}
		transcode_album_release(atoi(album_release_id), format);
	}
}

void route_form_without_session(const char* form, struct route_parameters* parameters) {
	if (!strcmp(form, "login")) {
		route_form_login(parameters->result, &parameters->data);
	} else if (!strcmp(form, "register")) {
		route_form_register(&parameters->data);
	}
}

void route_form(struct route_parameters* parameters) {
	if (!parameters->resource) {
		return;
	}
	char form[32];
	parameters->resource = split_string(form, sizeof(form), parameters->resource, '/');
	parameters->data = http_load_data(parameters->body, parameters->size, parameters->result->client->headers.content_type);
	if (parameters->session) {
		route_form_with_session(form, parameters);
	} else {
		route_form_without_session(form, parameters);
	}
	http_free_data(&parameters->data);
}
