#include "playlist.h"
#include "database.h"

#include <stdlib.h>
#include <string.h>

void load_playlist_list(struct playlist_list_data* list, const char* user) {
	const char* params[] = { user };
	PGresult* result = call_procedure("select * from get_playlists_by_user", params, 1);
	list->num_playlists = PQntuples(result);
	list->playlists = (struct playlist_item_data*)malloc(list->num_playlists * sizeof(struct playlist_item_data));
	if (list->playlists) {
		for (int i = 0; i < list->num_playlists; i++) {
			struct playlist_item_data* playlist = &list->playlists[i];
			playlist->id = atoi(PQgetvalue(result, i, 0));
			strcpy(playlist->name, PQgetvalue(result, i, 1));
		}
	}
	PQclear(result);
}
