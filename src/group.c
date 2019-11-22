#include "group.h"
#include "album.h"
#include "cache.h"
#include "config.h"
#include "database.h"
#include "files.h"
#include "format.h"
#include "render.h"
#include "stack.h"
#include "system.h"
#include "track.h"
#include "type.h"

#include <stdlib.h>
#include <string.h>

void edit_group_detail(int id, enum group_detail detail, const char* value) {
	if (detail < GROUP_DETAIL_NAME || detail > GROUP_DETAIL_DESCRIPTION) {
		print_error("Invalid group detail");
		return;
	}
	const char* fields[] = { "name", "country_code", "website", "description" };
	const char* query = push_string_f("update \"group\" set \"%s\" = $1 where \"id\" = $2", fields[detail]);
	char id_str[32];
	snprintf(id_str, 32, "%i", id);
	const char* params[] = { value, id_str };
	PGresult* result = execute_sql(query, params, 2);
	PQclear(result);
	pop_string();
}

void free_group(struct group_data* group) {
	free(group->tags);
	for (int i = 0; i < group->num_albums; i++) {
		free(group->albums[i].image);
	}
	free(group->albums);
	free(group->tracks);
}

void render_edit_group(struct render_buffer* buffer, int id) {
	struct group_data group;
	load_group(&group, id);
	if (strlen(group.name) > 0) {
		char* website = NULL;
		char* description = NULL;
		load_all_group_details(group.country, NULL, 0, &website, &description, id);
		assign_buffer(buffer, get_cached_file("html/edit_group.html", NULL));
		set_parameter(buffer, "name", group.name); // header
		set_parameter_int(buffer, "group-id", id);
		set_parameter(buffer, "name", group.name); // input
		struct search_data country;
		memset(&country, 0, sizeof(country));
		strcpy(country.name, "country");
		strcpy(country.type, "country");
		strcpy(country.value, group.country);
		strcpy(country.text, get_country_name(group.country));
		render_search(buffer, "country_search", &country);
		set_parameter(buffer, "website", website ? website : "");
		set_parameter(buffer, "description", description ? description : "");
		free(website);
		free(description);
	}
	free_group(&group);
}

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

void render_group(struct render_buffer* buffer, int id, bool edit_tags, bool edit_details, bool favourited) {
	struct group_data group;
	load_group(&group, id);
	if (strlen(group.name) > 0) {
		assign_buffer(buffer, get_cached_file("html/group.html", NULL));
		set_parameter(buffer, "name", group.name);
		set_parameter_int(buffer, "group-id", id);
		set_parameter(buffer, "favourited", favourited ? "&#9733;" : "&#9734;");
		if (edit_details) {
			set_parameter(buffer, "edit", get_cached_file("html/edit_group_link.html", NULL));
			set_parameter_int(buffer, "group-id", id);
		} else {
			set_parameter(buffer, "edit", "");
		}
		render_tags(buffer, "tags", group.tags, group.num_tags);
		if (edit_tags) {
			const char* button = get_cached_file("html/edit_group_tags_button.html", NULL);
			set_parameter(buffer, "edit-tags", button);
			set_parameter_int(buffer, "group-id", id);
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
	free_group(&group);
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

void load_group_name(char* name, int size, int id) {
	char id_str[32];
	sprintf(id_str, "%i", id);
	const char* params[] = { id_str };
	PGresult* result = execute_sql("select \"name\" from \"group\" where \"id\" = $1", params, 1);
	if (PQntuples(result) > 0) {
		snprintf(name, size, "%s", PQgetvalue(result, 0, 0));
	} else {
		*name = '\0';
	}
	PQclear(result);
}

void load_all_group_details(char* country, char* name, int name_size, char** website, char** description, int id) {
	char id_str[32];
	sprintf(id_str, "%i", id);
	const char* params[] = { id_str };
	PGresult* result = execute_sql("select \"country_code\", \"name\", \"website\", \"description\" from \"group\" where \"id\" = $1", params, 1);
	if (PQntuples(result) > 0) {
		snprintf(country, 4, "%s", PQgetvalue(result, 0, 0));
		if (name) {
			snprintf(name, name_size, "%s", PQgetvalue(result, 0, 1));
		}
		*website = copy_string(PQgetvalue(result, 0, 2));
		*description = copy_string(PQgetvalue(result, 0, 3));
	} else {
		*country = '\0';
		if (name) {
			*name = '\0';
		}
		*website = NULL;
		*description = NULL;
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
			snprintf((*tags)[i].name, 64, "%s", PQgetvalue(result, i, 0));
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
			const int album_id = atoi(PQgetvalue(result, i, 0));
			const int album_release_id = atoi(PQgetvalue(result, i, 4));
			const char* released_at = PQgetvalue(result, i, 7);
			const char* name = PQgetvalue(result, i, 1);
			const char* type = PQgetvalue(result, i, 2);
			const int cover = PQgetisnull(result, i, 3) ? 0 : atoi(PQgetvalue(result, i, 3));
			initialize_album(&(*albums)[i], album_id, album_release_id, released_at, name, type, cover);
		}
	}
	PQclear(result);
}

void load_group(struct group_data* group, int id) {
	load_group_name(group->name, 128, id);
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

bool is_group_favourited(const char* user_name, int group_id) {
	char group_id_str[32];
	snprintf(group_id_str, sizeof(group_id_str), "%i", group_id);
	const char* params[] = { user_name, group_id_str };
	PGresult* result = execute_sql("select \"num\" from \"favourite_group\" where \"user_name\" = $1 and \"group_id\" = $2", params, 2);
	const bool favourited = PQntuples(result) != 0;
	PQclear(result);
	return favourited;
}

int get_next_group_favourite_num(const char* user_name) {
	const char* params[] = { user_name };
	PGresult* result = execute_sql("select max(\"num\") from \"favourite_group\" where \"user_name\" = $1", params, 1);
	int total = 0;
	if (PQntuples(result) == 1) {
		total = atoi(PQgetvalue(result, 0, 0));
	}
	PQclear(result);
	return total + 1;
}

void add_group_favourite(const char* user_name, int group_id) {
	char group_id_str[32];
	snprintf(group_id_str, sizeof(group_id_str), "%i", group_id);
	char num_str[32];
	const int next_num = get_next_group_favourite_num(user_name);
	snprintf(num_str, sizeof(num_str), "%i", next_num);
	const char* params[] = { user_name, group_id_str, num_str };
	insert_row("favourite_group", false, 3, params);
}

void remove_group_favourite(const char* user_name, int group_id) {
	char group_id_str[32];
	snprintf(group_id_str, sizeof(group_id_str), "%i", group_id);
	const char* params[] = { user_name, group_id_str };
	PGresult* result = execute_sql("delete from \"favourite_group\" where \"user_name\" = $1 and \"group_id\" = $2", params, 2);
	PQclear(result);
}
