#pragma once

#include "search.h"

struct album_data;
struct track_data;

struct tag_data {
	char name[64];
};

struct group_data {
	char name[128];
	int num_tags;
	int num_albums;
	int num_tracks;
	struct tag_data* tags;
	struct album_data* albums;
	struct track_data* tracks;
};

struct group_thumb_data {
	int id;
	char name[128];
	char* image;
};

struct add_group_data {
	struct search_data country;
	struct search_data person;
};

void render_add_group(struct render_buffer* buffer);
void render_group_list(struct render_buffer* buffer);
void render_group(struct render_buffer* buffer, int id);
void load_add_group(struct add_group_data* add);
void load_group_thumbs(struct group_thumb_data** thumbs, int* num_thumbs);
void load_group_name(char* dest, int id);
void load_group_tags(struct tag_data** tags, int* count, int id);
void load_group_tracks(struct track_data** tracks, int* count, int group_id);
void load_group_albums(struct album_data** albums, int* count, int group_id);
void load_group(struct group_data* group, int id);
