#include "database.h"
#include "http.h"
#include "route.h"
#include "session.h"
#include "session_track.h"
#include "stack.h"
#include "system.h"
#include "track.h"

#include <stdlib.h>

void route_form_add_session_track(struct route_parameters* parameters) {
	int album_release_id = atoi(http_data_string(&parameters->data, "album"));
	if (album_release_id == 0) {
		print_error("Album release must be specified when adding a session track");
		return;
	}
	int disc_num = atoi(http_data_string(&parameters->data, "disc"));
	int track_num = atoi(http_data_string(&parameters->data, "track"));
	int from_num = 0;
	int to_num = 0;
	if (track_num > 0) {
		from_num = create_session_track(parameters->session->name, album_release_id, disc_num, track_num);
	} else if (disc_num > 0) {
		struct track_data* tracks = NULL;
		int count = 0;
		load_disc_tracks(&tracks, &count, album_release_id, disc_num);
		for (int i = 0; i < count; i++) {
			to_num = create_session_track(parameters->session->name, album_release_id, disc_num, tracks[i].num);
			if (from_num == 0) {
				from_num = to_num;
			}
		}
		free(tracks);
	} else {
		struct track_data* tracks = NULL;
		int count = 0;
		load_album_tracks(&tracks, &count, album_release_id);
		for (int i = 0; i < count; i++) {
			to_num = create_session_track(parameters->session->name, album_release_id, tracks[i].disc_num, tracks[i].num);
			if (from_num == 0) {
				from_num = to_num;
			}
		}
		free(tracks);
	}
	if (from_num == 0) {
		print_error("Failed to add session track.");
		return;
	}
	parameters->resource = push_string_f("session-tracks/%i/%i", from_num, to_num);
	route_render(parameters);
	pop_string();
}

void route_form_delete_session_track(struct route_parameters* parameters) {
	const char* num = http_data_string(&parameters->data, "num");
	if (*num) {
		const char* params[] = { parameters->session->name, num };
		PQclear(call_procedure("call delete_session_track", params, 2));
	} else {
		print_error("Index must be specified when deleting a session track");
	}
}
