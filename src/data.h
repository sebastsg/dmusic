#pragma once

#define _XOPEN_SOURCE 500

#include <stddef.h>
#include <stdbool.h>

char* render_resource(bool is_main, const char* resource);

struct select_option {
	char code[64];
	char name[128];
};

struct select_options {
	struct select_option* options;
	int count;
};

struct tag_data {
	char name[64];
};

struct track_data {
	int album_release_id;
	int disc_num;
	int num;
	char name[128];
};

struct album_data {
	int id;
	int album_release_id;
	char released_at[16];
	char name[128];
	char image[128];
	char album_type_code[64];
	int num_discs;
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

struct search_data {
	char name[64];
	char type[32];
	char value[128];
	char text[128];
};

struct search_result_data {
	char value[128];
	char text[128];
};

struct prepare_attachment_data {
	char name[128];
	char link[256];
	char preview[256];
	char path[256];
	struct select_options targets;
	int selected_target;
};

struct prepare_track_data {
	int num;
	char fixed[128];
	char uploaded[256];
	char extension[16];
	char path[256];
};

struct prepare_disc_data {
	int num;
	char name[128];
	struct prepare_track_data tracks[128];
	int num_tracks;
};

struct prepare_data {
	char filename[512]; // without timestamp prefix
	char folder[512]; // with timestamp prefix
	char album_name[256]; // guessed album name
	char released_at[64];
	struct search_data group_search;
	struct prepare_attachment_data attachments[50];
	struct prepare_disc_data discs[4];
	int num_attachments;
	int num_discs;
	char album_type[64];
	char album_release_type[64];
	char audio_format[32];
};

struct upload_uploaded_data {
	char prefix[32];
	char name[256];
};

struct upload_data {
	struct upload_uploaded_data* uploads;
	int num_uploads;
};

struct session_track_data {
	int album_release_id;
	int disc_num;
	int track_num;
	char name[128];
	char duration[16];
};

struct playlist_item_data {
	int id;
	char name[128];
};

struct playlist_list_data {
	struct playlist_item_data* playlists;
	int num_playlists;
};

struct add_group_data {
	struct search_data country;
	struct search_data person;
};

struct group_thumb_data {
	int id;
	char name[128];
	char image[128];
};

void load_prepare(struct prepare_data* prepare, const char* prefix);
void load_add_group(struct add_group_data* add);
void load_upload(struct upload_data* upload);
