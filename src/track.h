#pragma once

struct render_buffer;

struct track_data {
	int album_release_id;
	int disc_num;
	int num;
	char name[192];
};

void render_track(struct render_buffer* buffer, struct track_data* track);
void load_disc_tracks(struct track_data** tracks, int* count, int album_release_id, int disc_num);
void load_album_tracks(struct track_data** tracks, int* count, int album_release_id);
void update_track_duration(int album_release_id, int disc_num, int track_num);
void update_disc_track_durations(int album_release_id, int disc_num);
void update_album_track_durations(int album_release_id);
void update_all_track_durations();
