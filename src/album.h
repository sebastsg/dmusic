#pragma once

#include <stdbool.h>

struct render_buffer;
struct track_data;

struct album_data {
	int id;
	int album_release_id;
	char released_at[16];
	char name[128];
	char* image;
	char album_type_code[64];
	int num_discs;
};

void render_albums(struct render_buffer* buffer, const char* type, const char* name, struct album_data* albums, int num_albums, struct track_data* tracks, int num_tracks);
void render_album(struct render_buffer* buffer, struct album_data* album, struct track_data* tracks, int num_tracks);
bool render_disc(struct render_buffer* buffer, int album_release_id, int disc_num, struct track_data* tracks, int num_tracks);
void initialize_album(struct album_data* album, int album_id, int album_release_id, const char* released_at, const char* name, const char* type, int cover);
void load_album_release(struct album_data* album, int album_release_id);
