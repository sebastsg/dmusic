#pragma once

struct render_buffer;
struct cached_session;

struct session_track_data {
	int album_release_id;
	int disc_num;
	int track_num;
	char name[128];
	char duration[16];
};

int create_session_track(const char* user, int album_release_id, int disc_num, int track_num);
void load_session_tracks(struct session_track_data** tracks, int* count, const char* user);
void delete_session_tracks(const char* user);
void render_session_tracks_array(struct render_buffer* buffer, const char* key, struct session_track_data* tracks, int num_tracks);
void render_session_tracks_database(struct render_buffer* buffer, const struct cached_session* session, int from_num, int to_num);
