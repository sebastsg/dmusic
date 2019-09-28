#include "group.h"
#include "render.h"
#include "cache.h"
#include "database.h"
#include "album.h"
#include "track.h"
#include "type.h"
#include "config.h"
#include "format.h"
#include "stack.h"

#include <stdlib.h>
#include <string.h>

void render_add_group(struct render_buffer* buffer) {
	struct add_group_data add_group;
	load_add_group(&add_group);
	assign_buffer(buffer, get_cached_file("html/add_group.html", NULL));
	render_search(buffer, "country_search", &add_group.country);
	render_search(buffer, "person_search", &add_group.person);
	struct render_buffer select_buffer;
	init_render_buffer(&select_buffer, 2048);
	const struct select_options* roles = get_cached_options("role");
	for (int i = 0; i < roles->count; i++) {
		append_buffer(&select_buffer, "{{ option }}");
		render_select_option(&select_buffer, "option", roles->options[i].code, roles->options[i].name, 0);
	}
	set_parameter(buffer, "roles", select_buffer.data);
	free(select_buffer.data);
}

void render_group(struct render_buffer* buffer, int id) {
	struct group_data group;
	load_group(&group, id);
	if (strlen(group.name) > 0) {
		assign_buffer(buffer, get_cached_file("html/group.html", NULL));
		set_parameter(buffer, "name", group.name);
		render_tags(buffer, "tags", group.tags, group.num_tags);
		struct render_buffer album_list_buffer;
		init_render_buffer(&album_list_buffer, 2048);
		const struct select_options* album_types = get_cached_options("album_type");
		for (int i = 0; i < album_types->count; i++) {
			struct select_option* option = &album_types->options[i];
			render_albums(&album_list_buffer, option->code, option->name, group.albums, group.num_albums, group.tracks, group.num_tracks);
		}
		set_parameter(buffer, "albums", album_list_buffer.data);
		free(album_list_buffer.data);
	}
	free(group.tags);
	for (int i = 0; i < group.num_albums; i++) {
		free(group.albums[i].image);
	}
	free(group.albums);
	free(group.tracks);
}

void render_group_thumb(struct render_buffer* buffer, const char* key, int id, const char* name, const char* image) {
	set_parameter(buffer, key, get_cached_file("html/group_thumb.html", NULL));
	set_parameter_int(buffer, "id", id);
	set_parameter(buffer, "image", image);
	set_parameter(buffer, "name", name);
}

void render_group_thumb_list(struct render_buffer* buffer, struct group_thumb_data* thumbs, int num_thumbs) {
	struct render_buffer item_buffer;
	init_render_buffer(&item_buffer, 512);
	for (int i = 0; i < num_thumbs; i++) {
		append_buffer(&item_buffer, "{{ thumb }}");
		render_group_thumb(&item_buffer, "thumb", thumbs[i].id, thumbs[i].name, thumbs[i].image);
	}
	strcpy(buffer->data, get_cached_file("html/groups.html", NULL));
	set_parameter(buffer, "thumbs", item_buffer.data);
	free(item_buffer.data);
}

void render_group_list(struct render_buffer* buffer) {
	struct group_thumb_data* thumbs = NULL;
	int num_thumbs = 0;
	load_group_thumbs(&thumbs, &num_thumbs);
	render_group_thumb_list(buffer, thumbs, num_thumbs);
	for (int i = 0; i < num_thumbs; i++) {
		free(thumbs[i].image);
	}
	free(thumbs);
}

void load_add_group(struct add_group_data* add) {
	strcpy(add->country.name, "country");
	strcpy(add->country.type, "country");
	strcpy(add->country.value, "--");
	strcpy(add->country.text, "Unknown");
	strcpy(add->person.name, "id");
	strcpy(add->person.type, "person");
	strcpy(add->person.value, "0");
	strcpy(add->person.text, "");
}

void load_group_thumbs(struct group_thumb_data** thumbs, int* num_thumbs) {
	PGresult* result = execute_sql("select \"id\", name from \"group\"", NULL, 0);
	*num_thumbs = PQntuples(result);
	*thumbs = (struct group_thumb_data*)malloc(*num_thumbs * sizeof(struct group_thumb_data));
	if (*thumbs) {
		for (int i = 0; i < *num_thumbs; i++) {
			struct group_thumb_data* thumb = &(*thumbs)[i];
			thumb->id = atoi(PQgetvalue(result, i, 0));
			strcpy(thumb->name, PQgetvalue(result, i, 1));
			thumb->image = copy_string(client_group_image_path(thumb->id, 1));
		}
	}
	PQclear(result);
}

void load_group_name(char* dest, int id) {
	char id_str[32];
	sprintf(id_str, "%i", id);
	const char* params[] = { id_str };
	PGresult* result = execute_sql("select name from \"group\" where \"id\" = $1", params, 1);
	if (PQntuples(result) > 0) {
		strcpy(dest, PQgetvalue(result, 0, 0));
	} else {
		dest[0] = '\0';
	}
	PQclear(result);
}

void load_group_tags(struct tag_data** tags, int* num_tags, int id) {
	char id_str[32];
	sprintf(id_str, "%i", id);
	const char* params[] = { id_str };
	PGresult* result = call_procedure("select * from get_group_tags", params, 1);
	*num_tags = PQntuples(result);
	*tags = (struct tag_data*)malloc(*num_tags * sizeof(struct tag_data));
	if (*tags) {
		for (int i = 0; i < *num_tags; i++) {
			strcpy((*tags)[i].name, PQgetvalue(result, i, 0));
		}
	}
	PQclear(result);
}

void load_group_tracks(struct track_data** tracks, int* num_tracks, int group_id) {
	char id_str[32];
	sprintf(id_str, "%i", group_id);
	const char* params[] = { id_str };
	PGresult* result = call_procedure("select * from get_group_tracks", params, 1);
	*num_tracks = PQntuples(result);
	*tracks = (struct track_data*)malloc(*num_tracks * sizeof(struct track_data));
	if (*tracks) {
		for (int i = 0; i < *num_tracks; i++) {
			struct track_data* track = &(*tracks)[i];
			track->album_release_id = atoi(PQgetvalue(result, i, 0));
			track->disc_num = atoi(PQgetvalue(result, i, 1));
			track->num = atoi(PQgetvalue(result, i, 2));
			strcpy(track->name, PQgetvalue(result, i, 4));
		}
	}
	PQclear(result);
}

void load_group_albums(struct album_data** albums, int* num_albums, int group_id) {
	char id_str[32];
	sprintf(id_str, "%i", group_id);
	const char* params[] = { id_str };
	PGresult* result = call_procedure("select * from get_group_albums", params, 1);
	*num_albums = PQntuples(result);
	*albums = (struct album_data*)malloc(*num_albums * sizeof(struct album_data));
	if (*albums) {
		for (int i = 0; i < *num_albums; i++) {
			struct album_data* album = &(*albums)[i];
			album->id = atoi(PQgetvalue(result, i, 0));
			album->album_release_id = atoi(PQgetvalue(result, i, 4));
			format_time(album->released_at, 16, "%Y", (time_t)atoi(PQgetvalue(result, i, 7)));
			strcpy(album->name, PQgetvalue(result, i, 1));
			strcpy(album->album_type_code, PQgetvalue(result, i, 2));
			int cover_num = (PQgetisnull(result, i, 3) ? 0 : atoi(PQgetvalue(result, i, 3)));
			if (cover_num != 0) {
				album->image = copy_string(client_album_image_path(album->album_release_id, cover_num));
			} else {
				album->image = copy_string("/img/missing.png");
			}
		}
	}
	PQclear(result);
}

void load_group(struct group_data* group, int id) {
	load_group_name(group->name, id);
	load_group_tags(&group->tags, &group->num_tags, id);
	load_group_albums(&group->albums, &group->num_albums, id);
	load_group_tracks(&group->tracks, &group->num_tracks, id);
}
