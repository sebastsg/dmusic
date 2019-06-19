#pragma once

#include "data.h"

#include <stdbool.h>
#include <stdint.h>

void connect_database();
void disconnect_database();

void execute_sql_file(const char* name, bool split);

long long insert_row(const char* table, bool has_serial_id, int argc, const char** argv);

void load_options(struct select_options* options, const char* type);
void load_search_results(struct search_result_data** results, int* count, const char* type, const char* query);
void load_group_name(char* dest, uint64_t id);
void load_group_tags(struct tag_data** tags, int* count, uint64_t id);
void load_group_tracks(struct track_data** tracks, int* count, uint64_t group_id);
void load_group_albums(struct album_data** albums, int* count, uint64_t group_id);
void load_group(struct group_data* group, uint64_t id);
void load_session_tracks(struct session_track_data** tracks, int* count, const char* user);
void load_playlist_list(struct playlist_list_data* list, const char* user);
void load_group_thumbs(struct group_thumb_data** thumbs, int* num_thumbs);
