#pragma once


struct playlist_item_data {
	int id;
	char name[128];
};

struct playlist_list_data {
	struct playlist_item_data* playlists;
	int num_playlists;
};

void load_playlist_list(struct playlist_list_data* list, const char* user);
