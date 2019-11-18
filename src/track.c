#include "track.h"
#include "database.h"
#include "cache.h"
#include "render.h"
#include "system.h"
#include "config.h"

#include <string.h>
#include <stdlib.h>

void render_track(struct render_buffer* buffer, struct track_data* track) {
	append_buffer(buffer, get_cached_file("html/track.html", NULL));
	const char* track_url = client_track_path(track->album_release_id, track->disc_num, track->num);
	set_parameter(buffer, "queue_url", track_url);
	set_parameter(buffer, "space_prefix", track->num < 10 ? "&nbsp;" : "");
	set_parameter_int(buffer, "num_str", track->num);
	set_parameter(buffer, "play_url", track_url);
	set_parameter(buffer, "name", track->name);
}

void load_disc_tracks(struct track_data** tracks, int* count, int album_release_id, int disc_num) {
	char album_release_id_str[32];
	char disc_num_str[32];
	sprintf(album_release_id_str, "%i", album_release_id);
	sprintf(disc_num_str, "%i", disc_num);
	const char* params[] = { album_release_id_str, disc_num_str };
	PGresult* result = execute_sql("select \"num\", \"name\" from \"track\" where \"album_release_id\" = $1 and \"disc_num\" = $2", params, 2);
	*count = PQntuples(result);
	*tracks = (struct track_data*)malloc(*count * sizeof(struct track_data));
	if (*tracks) {
		for (int i = 0; i < *count; i++) {
			struct track_data* track = &(*tracks)[i];
			track->album_release_id = album_release_id;
			track->disc_num = disc_num;
			track->num = atoi(PQgetvalue(result, i, 0));
			strcpy(track->name, PQgetvalue(result, i, 1));
		}
	}
	PQclear(result);
}

void load_album_tracks(struct track_data** tracks, int* count, int album_release_id) {
	char album_release_id_str[32];
	sprintf(album_release_id_str, "%i", album_release_id);
	const char* params[] = { album_release_id_str };
	PGresult* result = call_procedure("select * from get_album_release_tracks", params, 1);
	*count = PQntuples(result);
	if (*count == 0) {
		print_error_f("Did not find any tracks for album release %i", album_release_id);
	}
	*tracks = (struct track_data*)malloc(*count * sizeof(struct track_data));
	if (*tracks) {
		for (int i = 0; i < *count; i++) {
			struct track_data* track = &(*tracks)[i];
			track->album_release_id = album_release_id;
			track->disc_num = atoi(PQgetvalue(result, i, 0));
			track->num = atoi(PQgetvalue(result, i, 1));
			strcpy(track->name, PQgetvalue(result, i, 3));
		}
	}
	PQclear(result);
}

void update_track_duration(int album_release_id, int disc_num, int track_num) {
	char format[16];
	find_best_audio_format(format, album_release_id, true);
	if (format[0] == '\0') {
		print_error_f("Did not find any file for track %i in disc %i in album release %i", track_num, disc_num, album_release_id);
		return;
	}
	const char* path = server_track_path(format, album_release_id, disc_num, track_num);
	char soxi_command[1024];
	sprintf(soxi_command, "soxi -D \"%s\"", path);
	char* soxi_out = system_output(soxi_command, NULL);
	if (!soxi_out) {
		print_error_f("Failed to run command: %s", soxi_command);
		return;
	}
	char seconds[32];
	sprintf(seconds, "%i", atoi(soxi_out));
	free(soxi_out);
	char album_release_id_str[32];
	char disc_num_str[32];
	char track_num_str[32];
	sprintf(album_release_id_str, "%i", album_release_id);
	sprintf(disc_num_str, "%i", disc_num);
	sprintf(track_num_str, "%i", track_num);
	const char* params[] = { seconds, album_release_id_str, disc_num_str, track_num_str };
	const char* query = "update \"track\" set \"seconds\" = $1 where \"album_release_id\" = $2 and \"disc_num\" = $3 and \"num\" = $4";
	PQclear(execute_sql(query, params, 4));
}

void update_disc_track_durations(int album_release_id, int disc_num) {
	char album_str[32];
	char disc_str[32];
	sprintf(album_str, "%i", album_release_id);
	sprintf(disc_str, "%i", disc_num);
	const char* params[] = { album_str, disc_str };
	PGresult* result = execute_sql("select \"num\" from \"track\" where \"album_release_id\" = $1 and \"disc_num\" = $2", params, 2);
	if (!result) {
		print_error_f("Did not find any tracks for disc %i in album release %i", disc_num, album_release_id);
		return;
	}
	for (int i = 0; i < PQntuples(result); i++) {
		update_track_duration(album_release_id, disc_num, atoi(PQgetvalue(result, i, 0)));
	}
	PQclear(result);
}

void update_album_track_durations(int album_release_id) {
	print_info_f("Updating track durations for album release %i", album_release_id);
	char str[32];
	sprintf(str, "%i", album_release_id);
	const char* params[] = { str };
	PGresult* result = execute_sql("select \"num\" from \"disc\" where \"album_release_id\" = $1", params, 1);
	if (!result) {
		print_error_f("Did not find any discs for album release %i", album_release_id);
		return;
	}
	for (int i = 0; i < PQntuples(result); i++) {
		update_disc_track_durations(album_release_id, atoi(PQgetvalue(result, i, 0)));
	}
	PQclear(result);
}

void update_all_track_durations() {
	PGresult* result = execute_sql("select \"id\" from \"album_release\"", NULL, 0);
	if (!result) {
		return;
	}
	for (int i = 0; i < PQntuples(result); i++) {
		update_album_track_durations(atoi(PQgetvalue(result, i, 0)));
	}
	PQclear(result);
}
