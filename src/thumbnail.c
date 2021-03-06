#include "thumbnail.h"

#include <stdlib.h>
#include <string.h>

#include "cache.h"
#include "config.h"
#include "database.h"
#include "render.h"
#include "search.h"
#include "stack.h"

typedef const char* (*get_thumbnail_link_path)(int, int, bool);

static void load_thumbnails_from_rows(struct thumbnail** thumbs, int* num_thumbs, PGresult* result, get_thumbnail_link_path link_path) {
	*num_thumbs = PQntuples(result);
	*thumbs = (struct thumbnail*)malloc(*num_thumbs * sizeof(struct thumbnail));
	if (*thumbs) {
		for (int i = 0; i < *num_thumbs; i++) {
			struct thumbnail* thumb = &(*thumbs)[i];
			thumb->id = atoi(PQgetvalue(result, i, 0));
			strcpy(thumb->name, PQgetvalue(result, i, 1));
			thumb->image = copy_string(link_path(thumb->id, 1, true));
		}
	}
}

void render_thumbnail(struct render_buffer* buffer, const char* type, int id, const char* name, const char* image) {
	append_buffer(buffer, get_cached_file("html/thumbnail.html", NULL));
	set_parameter(buffer, "type", type);
	set_parameter_int(buffer, "id", id);
	set_parameter(buffer, "type", type);
	set_parameter(buffer, "image", image);
	set_parameter(buffer, "name", name);
}

void load_all_group_thumbs(struct thumbnail** thumbs, int* num_thumbs) {
	PGresult* result = execute_sql("select \"id\", \"name\" from \"group\"", NULL, 0);
	load_thumbnails_from_rows(thumbs, num_thumbs, result, client_group_image_path);
	PQclear(result);
}

void load_recent_group_thumbs(struct thumbnail** thumbs, int* num_thumbs) {
	PGresult* result = execute_sql("select \"id\", \"name\" from \"group\" order by \"id\" desc limit 12", NULL, 0);
	load_thumbnails_from_rows(thumbs, num_thumbs, result, client_group_image_path);
	PQclear(result);
}

void load_recent_album_thumbs(struct thumbnail** thumbs, int* num_thumbs) {
	PGresult* result = execute_sql("select \"id\", \"name\" from \"album\" order by \"id\" desc limit 12", NULL, 0);
	load_thumbnails_from_rows(thumbs, num_thumbs, result, client_album_image_path);
	PQclear(result);
}

void load_group_thumbs_from_search(struct thumbnail** thumbs, const struct search_result* results, int count) {
	*thumbs = (struct thumbnail*)malloc(count * sizeof(struct thumbnail));
	if (*thumbs) {
		for (int i = 0; i < count; i++) {
			struct thumbnail* thumb = &(*thumbs)[i];
			thumb->id = atoi(results[i].value);
			strcpy(thumb->name, results[i].text);
			thumb->image = copy_string(client_group_image_path(thumb->id, 1, true));
		}
	}
}

void load_favourite_group_thumbs(struct thumbnail** thumbs, int* num_thumbs, const char* user_name) {
	const char* params[] = { user_name };
	PGresult* result = call_procedure("select * from get_favourite_groups", params, 1);
	load_thumbnails_from_rows(thumbs, num_thumbs, result, client_group_image_path);
	PQclear(result);
}

struct render_buffer render_thumbnail_list(struct thumbnail* thumbs, int num_thumbs, const char* type) {
	struct render_buffer buffer;
	init_render_buffer(&buffer, 512);
	for (int i = 0; i < num_thumbs; i++) {
		render_thumbnail(&buffer, type, thumbs[i].id, thumbs[i].name, thumbs[i].image);
	}
	return buffer;
}

void render_all_group_thumbnails(struct render_buffer* buffer) {
	struct thumbnail* thumbs = NULL;
	int num_thumbs = 0;
	load_all_group_thumbs(&thumbs, &num_thumbs);
	struct render_buffer list_buffer = render_thumbnail_list(thumbs, num_thumbs, "group");
	set_parameter(buffer, "group-thumbs", list_buffer.data);
	free(list_buffer.data);
	for (int i = 0; i < num_thumbs; i++) {
		free(thumbs[i].image);
	}
	free(thumbs);
}

void render_recent_group_thumbnails(struct render_buffer* buffer) {
	struct thumbnail* thumbs = NULL;
	int num_thumbs = 0;
	load_recent_group_thumbs(&thumbs, &num_thumbs);
	struct render_buffer list_buffer = render_thumbnail_list(thumbs, num_thumbs, "group");
	set_parameter(buffer, "group-thumbs", list_buffer.data);
	free(list_buffer.data);
	for (int i = 0; i < num_thumbs; i++) {
		free(thumbs[i].image);
	}
	free(thumbs);
}

void render_recent_album_thumbnails(struct render_buffer* buffer) {
	struct thumbnail* thumbs = NULL;
	int num_thumbs = 0;
	load_recent_album_thumbs(&thumbs, &num_thumbs);
	struct render_buffer list_buffer = render_thumbnail_list(thumbs, num_thumbs, "album");
	set_parameter(buffer, "album-thumbs", list_buffer.data);
	free(list_buffer.data);
	for (int i = 0; i < num_thumbs; i++) {
		free(thumbs[i].image);
	}
	free(thumbs);
}

void render_group_thumbnails_from_search(struct render_buffer* buffer, const char* type, const char* query) {
	struct thumbnail* thumbs = NULL;
	int num_thumbs = 0;
	struct search_result* results = NULL;
	load_search_results(&results, &num_thumbs, "groups", query);
	load_group_thumbs_from_search(&thumbs, results, num_thumbs);
	struct render_buffer list_buffer = render_thumbnail_list(thumbs, num_thumbs, "group");
	set_parameter(buffer, "group-thumbs", list_buffer.data);
	free(list_buffer.data);
	for (int i = 0; i < num_thumbs; i++) {
		free(thumbs[i].image);
	}
	free(thumbs);
	free(results);
}

void render_favourite_group_thumbnails(struct render_buffer* buffer, const char* user_name) {
	struct thumbnail* thumbs = NULL;
	int num_thumbs = 0;
	load_favourite_group_thumbs(&thumbs, &num_thumbs, user_name);
	struct render_buffer list_buffer = render_thumbnail_list(thumbs, num_thumbs, "group");
	set_parameter(buffer, "group-thumbs", list_buffer.data);
	free(list_buffer.data);
	for (int i = 0; i < num_thumbs; i++) {
		free(thumbs[i].image);
	}
	free(thumbs);
}
