#include "search.h"
#include "database.h"
#include "system.h"
#include "render.h"
#include "cache.h"

#include <stdlib.h>
#include <string.h>

static void load_search_results_from_pg_result(struct search_result** results, int* count, PGresult* result) {
	*count = PQntuples(result);
	if (*count > 0) {
		*results = (struct search_result*)malloc(*count * sizeof(struct search_result));
		if (*results) {
			for (int i = 0; i < *count; i++) {
				strcpy((*results)[i].value, PQgetvalue(result, i, 0));
				strcpy((*results)[i].text, PQgetvalue(result, i, 1));
			}
		}
	}
}

void load_search_results(struct search_result** results, int* count, const char* type, const char* query) {
	if (!strcmp(type, "groups") || !strcmp(type, "tag") || !strcmp(type, "person") || !strcmp(type, "country")) {
		char search[256];
		snprintf(search, sizeof(search), "%%%s%%", query);
		const char* limit = "10";
		const char* params[] = { search, limit };
		char procedure[32];
		sprintf(procedure, "select * from search_%s", type);
		PGresult* result = call_procedure(procedure, params, 2);
		load_search_results_from_pg_result(results, count, result);
		PQclear(result);
	} else {
		print_error_f("Invalid search type: " A_CYAN "%s", type);
	}
}

void render_search(struct render_buffer* buffer, const char* key, struct search_data* search) {
	set_parameter(buffer, key, get_cached_file("html/search.html", NULL));
	set_parameter(buffer, "name", search->name);
	set_parameter(buffer, "value", search->value);
	set_parameter(buffer, "text", search->text);
	set_parameter(buffer, "type", search->type);
	set_parameter(buffer, "render", search->render);
	set_parameter(buffer, "results", search->results);
}

void render_search_results(struct render_buffer* buffer, struct search_result* results, int num_results) {
	struct render_buffer result_buffer;
	init_render_buffer(&result_buffer, 2048);
	for (int i = 0; i < num_results; i++) {
		append_buffer(&result_buffer, get_cached_file("html/search_result.html", NULL));
		set_parameter(&result_buffer, "value", results[i].value);
		set_parameter(&result_buffer, "text", results[i].text);
	}
	assign_buffer(buffer, result_buffer.data);
	free(result_buffer.data);
}

void render_search_query(struct render_buffer* buffer, const char* type, const char* query) {
	struct search_result* results = NULL;
	int num_results = 0;
	if (!strcmp(type, "explore")) {
		// todo: albums, tracks, lyrics, tags...
		load_search_results(&results, &num_results, "groups", query);
	} else {
		load_search_results(&results, &num_results, type, query);
	}
	render_search_results(buffer, results, num_results);
	free(results);
}
