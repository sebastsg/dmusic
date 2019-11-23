#pragma once

void load_config();
void free_config();
const char* get_property(const char* key);

const char* server_root_path(const char* path);
const char* server_seed_path(const char* name);
const char* server_sql_path(const char* name);
const char* server_html_path(const char* name);
const char* server_cache_path(const char* path);
const char* server_group_path(int group_id);
const char* server_album_path(int album_release_id);

const char* server_disc_path(int album_release_id, int disc_num);
const char* server_disc_format_path(int album_release_id, int disc_num, const char* format);

const char* client_group_image_path(int group_id, int num);
const char* server_group_image_path(int group_id, int num);

const char* client_album_image_path(int album_release_id, int num);
const char* server_album_image_path(int album_release_id, int num);

const char* client_uploaded_file_path(const char* filename);
const char* server_uploaded_file_path(const char* filename);
const char* server_uploaded_directory_file_path(const char* directory, const char* filename);

const char* client_track_path(int album_release_id, int disc_num, int track_num);
const char* server_track_path(const char* format, int album_release_id, int disc_num, int track_num);
