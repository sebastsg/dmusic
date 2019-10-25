#include "session_track.h"
#include "render.h"
#include "database.h"
#include "session.h"
#include "cache.h"
#include "format.h"

#include <stdlib.h>
#include <string.h>

int create_session_track(const char* user, int album_release_id, int disc_num, int track_num) {
	char album[32];
	char disc[32];
	char track[32];
	sprintf(album, "%i", album_release_id);
	sprintf(disc, "%i", disc_num);
	sprintf(track, "%i", track_num);
	const char* params[] = { user, album, disc, track };
	PGresult* result = call_procedure("select * from create_session_track", params, 4);
	int num = (PQntuples(result) > 0) ? atoi(PQgetvalue(result, 0, 0)) : 0;
	PQclear(result);
	return num;
}

void load_session_tracks(struct session_track_data** tracks, int* num_tracks, const char* user) {
	const char* params[] = { user };
	PGresult* result = call_procedure("select * from get_session_tracks", params, 1);
	*num_tracks = PQntuples(result);
	if (*num_tracks == 0) {
		PQclear(result);
		return;
	}
	*tracks = (struct session_track_data*)malloc(*num_tracks * sizeof(struct session_track_data));
	if (*tracks) {
		for (int i = 0; i < *num_tracks; i++) {
			struct session_track_data* track = &(*tracks)[i];
			track->album_release_id = atoi(PQgetvalue(result, i, 0));
			track->disc_num = atoi(PQgetvalue(result, i, 1));
			track->track_num = atoi(PQgetvalue(result, i, 2));
			strcpy(track->name, PQgetvalue(result, i, 3));
			int seconds = atoi(PQgetvalue(result, i, 4));
			int minutes = seconds / 60;
			seconds = seconds % 60;
			make_duration_string(track->duration, minutes, seconds);
		}
	}
	PQclear(result);
}

void delete_session_tracks(const char* user) {
	const char* params[] = { user };
	PGresult* result = execute_sql("delete from \"session_track\" where \"user_name\" = $1", params, 1);
	PQclear(result);
}

void render_session_track(struct render_buffer* buffer, struct session_track_data* track) {
	append_buffer(buffer, get_cached_file("html/playlist_track.html", NULL));
	set_parameter_int(buffer, "album", track->album_release_id);
	set_parameter_int(buffer, "disc", track->disc_num);
	set_parameter_int(buffer, "track", track->track_num);
	set_parameter(buffer, "name", track->name);
	set_parameter(buffer, "duration", track->duration);
}

void render_session_tracks_array(struct render_buffer* buffer, const char* key, struct session_track_data* tracks, int num_tracks) {
	struct render_buffer item_buffer;
	init_render_buffer(&item_buffer, 512);
	for (int i = 0; i < num_tracks; i++) {
		render_session_track(&item_buffer, &tracks[i]);
	}
	set_parameter(buffer, key, item_buffer.data);
	free(item_buffer.data);
}

void render_session_tracks_database(struct render_buffer* buffer, const struct cached_session* session, int from_num, int to_num) {
	struct session_track_data* tracks = NULL;
	int num_tracks = 0;
	load_session_tracks(&tracks, &num_tracks, session->name);
	if (num_tracks > to_num || from_num > to_num) {
		to_num = num_tracks;
	}
	for (int num = from_num; num <= to_num; num++) {
		render_session_track(buffer, &tracks[num - 1]);
	}
	free(tracks);
}
