#pragma once

#include <stdbool.h>

struct select_options;
struct search_result_data;
struct tag_data;
struct track_data;
struct album_data;
struct group_data;
struct session_track_data;
struct playlist_list_data;
struct group_thumb_data;

void connect_database();
void disconnect_database();

void execute_sql_file(const char* name, bool split);

int insert_row(const char* table, bool has_serial_id, int argc, const char** argv);
int create_session_track(const char* user, int album_release_id, int disc_num, int track_num);
void delete_session_tracks(const char* user);

void load_options(struct select_options* options, const char* type);
void load_search_results(struct search_result_data** results, int* count, const char* type, const char* query);
void load_group_name(char* dest, int id);
void load_group_tags(struct tag_data** tags, int* count, int id);
void load_group_tracks(struct track_data** tracks, int* count, int group_id);
void load_group_albums(struct album_data** albums, int* count, int group_id);
void load_group(struct group_data* group, int id);
void load_disc_tracks(struct track_data** tracks, int* count, int album_release_id, int disc_num);
void load_album_tracks(struct track_data** tracks, int* count, int album_release_id);
void load_session_tracks(struct session_track_data** tracks, int* count, const char* user);
void load_playlist_list(struct playlist_list_data* list, const char* user);
void load_group_thumbs(struct group_thumb_data** thumbs, int* num_thumbs);
void find_best_audio_format(char* dest, int album_release_id, bool lossless);
void update_track_duration(int album_release_id, int disc_num, int track_num);
void update_disc_track_durations(int album_release_id, int disc_num);
void update_album_track_durations(int album_release_id);
void update_all_track_durations();

bool register_user(const char* name, const char* password);
bool login_user(const char* name, const char* password);
