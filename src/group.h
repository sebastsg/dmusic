#pragma once

#include "search.h"

#include <stdbool.h>

struct album_data;
struct track_data;

struct tag_data {
	char name[64];
};

struct group_data {
	char name[128];
	char country[4];
	int num_tags;
	int num_albums;
	int num_tracks;
	struct tag_data* tags;
	struct album_data* albums;
	struct track_data* tracks;
};

struct add_group_data {
	struct search_data country;
	struct search_data person;
};

enum group_detail {
	GROUP_DETAIL_NAME,
	GROUP_DETAIL_COUNTRY,
	GROUP_DETAIL_WEBSITE,
	GROUP_DETAIL_DESCRIPTION
};

void edit_group_detail(int id, enum group_detail detail, const char* value);

void render_edit_group(struct render_buffer* buffer, int id);
void render_add_group(struct render_buffer* buffer);
void render_group(struct render_buffer* buffer, int id, bool edit_tags, bool edit_details, bool favourited);
void render_group_tags(struct render_buffer* buffer, int id);
void render_edit_group_tags(struct render_buffer* buffer, int id);
void load_add_group(struct add_group_data* add);
void load_group_name(char* name, int size, int id);
void load_all_group_details(char* country, char* name, int name_size, char** website, char** description, int id);
void load_group_tags(struct tag_data** tags, int* count, int id);
void load_group_tracks(struct track_data** tracks, int* count, int group_id);
void load_group_albums(struct album_data** albums, int* count, int group_id);
void load_group(struct group_data* group, int id);
int create_group(const char* country, const char* name, const char* website, const char* description);
void create_group_tags(int group_id, const char* comma_separated_tags);
void delete_group_tag(int group_id, const char* tag);
void create_group_member(int group_id, int person_id, const char* role, const char* started_at, const char* ended_at);
bool is_group_favourited(const char* user_name, int group_id);
int get_next_group_favourite_num(const char* user_name);
void add_group_favourite(const char* user_name, int group_id);
void remove_group_favourite(const char* user_name, int group_id);
void add_group_alias(int group_id, const char* alias);
void delete_group_alias(int group_id, const char* alias);
void delete_all_group_aliases(int group_id);
