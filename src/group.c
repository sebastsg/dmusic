#include "group.h"
#include "render.h"
#include "cache.h"
#include "database.h"
#include "album.h"
#include "track.h"
#include "type.h"
#include "config.h"
#include "format.h"
#include "files.h"
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

void render_group_tags(struct render_buffer* buffer, int id) {
	struct tag_data* tags = NULL;
	int num_tags = 0;
	load_group_tags(&tags, &num_tags, id);
	assign_buffer(buffer, "{{ tags }}");
	render_tags(buffer, "tags", tags, num_tags);
	if (true) {
		append_buffer(buffer, get_cached_file("html/edit_group_tags_button.html", NULL));
		set_parameter_int(buffer, "group", id);
	}
	free(tags);
}

void render_group(struct render_buffer* buffer, int id, bool edit_tags) {
	struct group_data group;
	load_group(&group, id);
	if (strlen(group.name) > 0) {
		assign_buffer(buffer, get_cached_file("html/group.html", NULL));
		set_parameter(buffer, "name", group.name);
		render_tags(buffer, "tags", group.tags, group.num_tags);
		if (edit_tags) {
			const char* button = get_cached_file("html/edit_group_tags_button.html", NULL);
			set_parameter(buffer, "edit-tags", button);
			set_parameter_int(buffer, "group", id);
		} else {
			set_parameter(buffer, "edit-tags", "");
		}
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

void render_edit_group_tags(struct render_buffer* buffer, int id) {
	struct tag_data* tags = NULL;
	int num_tags = 0;
	load_group_tags(&tags, &num_tags, id);
	struct render_buffer tags_buffer;
	init_render_buffer(&tags_buffer, 1024);
	for (int i = 0; i < num_tags; i++) {
		append_buffer(&tags_buffer, get_cached_file("html/tag_edit_row.html", NULL));
		set_parameter_int(&tags_buffer, "group", id);
		set_parameter(&tags_buffer, "tag", tags[i].name);
		set_parameter(&tags_buffer, "tag", tags[i].name);
	}
	assign_buffer(buffer, get_cached_file("html/edit_group_tags.html", NULL));
	set_parameter(buffer, "tags", tags_buffer.data);
	struct search_data search;
	strcpy(search.name, "tag");
	strcpy(search.type, "tag");
	*search.value = '\0';
	*search.text = '\0';
	render_search(buffer, "search", &search);
	set_parameter_int(buffer, "group", id);
	set_parameter_int(buffer, "group", id);
	free(tags);
	free(tags_buffer.data);
}

void load_add_group(struct add_group_data* add) {
	memset(add, 0, sizeof(struct add_group_data));
	strcpy(add->country.name, "country");
	strcpy(add->country.type, "country");
	strcpy(add->country.value, "--");
	strcpy(add->country.text, "Unknown");
	strcpy(add->person.name, "id");
	strcpy(add->person.type, "person");
	strcpy(add->person.value, "0");
	strcpy(add->person.text, "");
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

PGresult* pg_load_group_tags(int group_id) {
	char id_str[32];
	sprintf(id_str, "%i", group_id);
	const char* params[] = { id_str };
	return call_procedure("select * from get_group_tags", params, 1);
}

void load_group_tags(struct tag_data** tags, int* num_tags, int id) {
	PGresult* result = pg_load_group_tags(id);
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

int create_group(const char* country, const char* name, const char* website, const char* description) {
	const char* params[] = { country, name, website, description };
	int group_id = insert_row("group", true, 4, params);
	if (group_id > 0) {
		create_directory(server_group_path(group_id));
	}
	return group_id;
}

void create_group_tags(int group_id, const char* comma_separated_tags) {
	char group_id_str[32];
	snprintf(group_id_str, sizeof(group_id_str), "%i", group_id);
	int priority = 1;
	PGresult* result = pg_load_group_tags(group_id);
	if (result) {
		const int count = PQntuples(result);
		if (count > 0) {
			priority += atoi(PQgetvalue(result, count - 1, 1));
		}
		PQclear(result);
	}
	char tag[128];
	while (comma_separated_tags = split_string(tag, sizeof(tag), comma_separated_tags, ',')) {
		char priority_str[32];
		sprintf(priority_str, "%i", priority);
		trim_ends(tag, " \t");
		const char* params[] = { group_id_str, tag, priority_str };
		insert_row("group_tag", false, 3, params);
		priority++;
	}
}

void delete_group_tag(int group_id, const char* tag) {
	char group_id_str[32];
	snprintf(group_id_str, sizeof(group_id_str), "%i", group_id);
	const char* params[] = { group_id_str, tag };
	PGresult* result = execute_sql("delete from \"group_tag\" where \"group_id\" = $1 and \"tag_name\" = $2", params, 2);
	PQclear(result);
}

void create_group_member(int group_id, int person_id, const char* role, const char* started_at, const char* ended_at) {
	char group_id_str[32];
	snprintf(group_id_str, sizeof(group_id_str), "%i", group_id);
	char person_id_str[32];
	snprintf(person_id_str, sizeof(person_id_str), "%i", person_id);
	const char* person_params[] = { group_id_str, person_id_str, role, started_at, ended_at };
	insert_row("group_member", false, 5, person_params);
}
