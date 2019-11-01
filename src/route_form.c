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

void route_form_toggle_edit_mode(struct cached_session* session) {
	toggle_preference(session, PREFERENCE_EDIT_MODE, EDIT_MODE_ON, EDIT_MODE_OFF);
	char preference[16];
	sprintf(preference, "%i", get_preference(session, PREFERENCE_EDIT_MODE));
	char type[16];
	sprintf(type, "%i", PREFERENCE_EDIT_MODE);
	const char* params[] = { session->name, type, preference };
	PGresult* result = call_procedure("call update_user_preference", params, 3);
	PQclear(result);
}

void route_form_logout(struct cached_session* session) {
	delete_session(session);
}

void route_form_with_session(const char* form, struct route_parameters* parameters) {
	if (!strcmp(form, "add-group")) {
		if (has_privilege(parameters->session, PRIVILEGE_ADD_GROUP)) {
			route_form_add_group(&parameters->data);
		}
	} else if (!strcmp(form, "upload")) {
		if (has_privilege(parameters->session, PRIVILEGE_UPLOAD_ALBUM)) {
			route_form_upload(&parameters->data);
		}
	} else if (!strcmp(form, "import")) {
		if (has_privilege(parameters->session, PRIVILEGE_IMPORT_ALBUM)) {
			route_form_import(&parameters->data);
		}
	} else if (!strcmp(form, "attach")) {
		if (has_privilege(parameters->session, PRIVILEGE_IMPORT_ALBUM)) {
			route_form_attach(parameters);
		}
	} else if (!strcmp(form, "add-session-track")) {
		route_form_add_session_track(parameters);
	} else if (!strcmp(form, "download-remote")) {
		if (has_privilege(parameters->session, PRIVILEGE_UPLOAD_ALBUM)) {
			route_form_download_remote(&parameters->data);
		}
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
	} else if (!strcmp(form, "add-group-tag")) {
		if (has_privilege(parameters->session, PRIVILEGE_EDIT_GROUP_TAGS)) {
			route_form_add_group_tag(parameters->result, &parameters->data);
		}
	} else if (!strcmp(form, "delete-group-tag")) {
		if (has_privilege(parameters->session, PRIVILEGE_EDIT_GROUP_TAGS)) {
			route_form_delete_group_tag(parameters->result, &parameters->data);
		}
	} else if (!strcmp(form, "toggle-edit-mode")) {
		route_form_toggle_edit_mode(parameters->session);
	} else if (!strcmp(form, "logout")) {
		route_form_logout(parameters->session);
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
