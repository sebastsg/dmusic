#include "album.h"

#include <stdlib.h>
#include <string.h>

#include "cache.h"
#include "config.h"
#include "database.h"
#include "format.h"
#include "render.h"
#include "stack.h"
#include "track.h"

void render_edit_disc(struct render_buffer* buffer, int album_release_id, int disc_num, struct track_data* tracks, int num_tracks) {
	//
}

void render_edit_album(struct render_buffer* buffer, struct album_data* album, struct track_data* tracks, int num_tracks) {
	append_buffer(buffer, get_cached_file("html/album.html", NULL));
	set_parameter(buffer, "name", album->name);
	set_parameter_int(buffer, "id", album->id);
	set_parameter(buffer, "released", album->released_at);
	set_parameter(buffer, "catalog", album->catalog);
	// groups:
}

void render_album(struct render_buffer* buffer, struct album_data* album, struct track_data* tracks, int num_tracks, bool edit_mode) {
	struct render_buffer discs_buffer;
	init_render_buffer(&discs_buffer, 2048);
	int i = 1;
	while (render_disc(&discs_buffer, album->album_release_id, i, tracks, num_tracks)) {
		i++;
	}
	append_buffer(buffer, get_cached_file("html/album.html", NULL));
	set_parameter(buffer, "image", album->image);
	set_parameter_int(buffer, "id_name", album->id);
	set_parameter(buffer, "name", album->name);
	set_parameter_int(buffer, "id_play", album->id);
	set_parameter_int(buffer, "id_queue", album->id);
	set_parameter_int(buffer, "id_download", album->id);
	if (edit_mode) {
		set_parameter(buffer, "edit", get_cached_file("html/edit_album_link.html", NULL));
		set_parameter_int(buffer, "album-release-id", album->id);
	} else {
		set_parameter(buffer, "edit", "");
	}
	set_parameter(buffer, "released", album->released_at);
	set_parameter(buffer, "discs", discs_buffer.data);
	free(discs_buffer.data);
}

void render_albums(struct render_buffer* buffer, const char* type, const char* name, struct album_data* albums, int num_albums, struct track_data* tracks, int num_tracks, bool edit_mode) {
	struct render_buffer item_buffer;
	init_render_buffer(&item_buffer, 2048);
	int count = 0;
	for (int i = 0; i < num_albums; i++) {
		if (!strcmp(type, albums[i].album_type_code)) {
			render_album(&item_buffer, &albums[i], tracks, num_tracks, edit_mode);
			count++;
		}
	}
	if (count > 0) {
		append_buffer(buffer, get_cached_file("html/albums.html", NULL));
		set_parameter(buffer, "title", name);
		set_parameter(buffer, "albums", item_buffer.data);
	}
	free(item_buffer.data);
}

bool render_disc(struct render_buffer* buffer, int album_release_id, int disc_num, struct track_data* tracks, int num_tracks) {
	struct render_buffer tracks_buffer;
	init_render_buffer(&tracks_buffer, 2048);
	int tracks_added = 0;
	int i = 0;
	while (i < num_tracks) {
		if (tracks[i].album_release_id != album_release_id || tracks[i].disc_num != disc_num) {
			i++;
			continue;
		}
		render_track(&tracks_buffer, &tracks[i]);
		i++;
		tracks_added++;
		if (tracks_added % 16 == 0) {
			append_buffer(buffer, get_cached_file("html/disc.html", NULL));
			set_parameter(buffer, "tracks", tracks_buffer.data);
			tracks_buffer.data[0] = '\0';
			tracks_buffer.offset = 0;
		}
	}
	if (tracks_added % 16 != 0) {
		append_buffer(buffer, get_cached_file("html/disc.html", NULL));
		set_parameter(buffer, "tracks", tracks_buffer.data);
	}
	free(tracks_buffer.data);
	return tracks_added > 0;
}

void initialize_album(struct album_data* album, int album_id, int album_release_id, const char* released_at, const char* name, const char* type, int cover, const char* catalog) {
	album->id = album_id;
	album->album_release_id = album_release_id;
	format_time(album->released_at, 16, "%Y", (time_t)atoi(released_at));
	snprintf(album->name, 128, "%s", name);
	snprintf(album->album_type_code, 64, "%s", type);
	if (cover != 0) {
		album->image = copy_string(client_album_image_path(album->album_release_id, cover, true));
	} else {
		album->image = copy_string("/img/missing.png");
	}
	snprintf(album->catalog, sizeof(album->catalog), "%s", catalog);
}

void load_album_release(struct album_data* album, int album_release_id) {
	memset(album, 0, sizeof(struct album_data));
	char album_release_id_str[32];
	snprintf(album_release_id_str, 32, "%i", album_release_id);
	const char* params[] = { album_release_id_str };
	PGresult* result = call_procedure("select * from get_album_release", params, 1);
	if (PQntuples(result) > 0) {
		const int album_id = atoi(PQgetvalue(result, 0, 0));
		const int album_release_id = atoi(PQgetvalue(result, 0, 4));
		const char* released_at = PQgetvalue(result, 0, 7);
		const char* name = PQgetvalue(result, 0, 1);
		const char* type = PQgetvalue(result, 0, 2);
		const char* catalog = PQgetvalue(result, 0, 6);
		const int cover = PQgetisnull(result, 0, 3) ? 0 : atoi(PQgetvalue(result, 0, 3));
		initialize_album(album, album_id, album_release_id, released_at, name, type, cover, catalog);
	}
	PQclear(result);
}
