#include "route.h"
#include "system.h"
#include "database.h"
#include "session.h"
#include "http.h"
#include "session_track.h"
#include "track.h"

#include <stdlib.h>

void route_form_add_session_track(struct route_result* result, struct http_data* data) {
	int album_release_id = atoi(http_data_string(data, "album"));
	if (album_release_id == 0) {
		print_error("Album release must be specified when adding a session track");
		return;
	}
	int disc_num = atoi(http_data_string(data, "disc"));
	int track_num = atoi(http_data_string(data, "track"));
	int from_num = 0;
	int to_num = 0;
	if (track_num > 0) {
		from_num = create_session_track(get_session_username(), album_release_id, disc_num, track_num);
	} else if (disc_num > 0) {
		struct track_data* tracks = NULL;
		int count = 0;
		load_disc_tracks(&tracks, &count, album_release_id, disc_num);
		for (int i = 0; i < count; i++) {
			to_num = create_session_track(get_session_username(), album_release_id, disc_num, tracks[i].num);
			if (from_num == 0) {
				from_num = to_num;
			}
		}
	} else {
		struct track_data* tracks = NULL;
		int count = 0;
		load_album_tracks(&tracks, &count, album_release_id);
		for (int i = 0; i < count; i++) {
			to_num = create_session_track(get_session_username(), album_release_id, tracks[i].disc_num, tracks[i].num);
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
	char resource[128];
	sprintf(resource, "session_tracks/%i/%i", from_num, to_num);
	route_render(result, resource);
}
