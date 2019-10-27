#include "search.h"
#include "database.h"
#include "system.h"
#include "render.h"
#include "cache.h"

#include <stdlib.h>
#include <string.h>

void load_search_results(struct search_result_data** search_results, int* count, const char* type, const char* query) {
	if (strcmp(type, "country") && strcmp(type, "groups") && strcmp(type, "person") && strcmp(type, "tag")) {
		print_error_f("Invalid search type: " A_CYAN "%s", type);
		return;
	}
	char procedure[64];
	sprintf(procedure, "select * from search_%s", type);
	char search[256];
	snprintf(search, sizeof(search), "%%%s%%", query);
	const char* limit = "10";
	const char* params[] = { search, limit };
	PGresult* result = call_procedure(procedure, params, 2);
	*count = PQntuples(result);
	if (*count == 0) {
		PQclear(result);
		return;
	}
	*search_results = (struct search_result_data*)malloc(*count * sizeof(struct search_result_data));
	if (!*search_results) {
		PQclear(result);
		return;
	}
	for (int i = 0; i < *count; i++) {
		struct search_result_data* search_result = &(*search_results)[i];
		strcpy(search_result->value, PQgetvalue(result, i, 0));
		strcpy(search_result->text, PQgetvalue(result, i, 1));
	}
	PQclear(result);
}

void render_search(struct render_buffer* buffer, const char* key, struct search_data* search) {
	set_parameter(buffer, key, get_cached_file("html/search.html", NULL));
	set_parameter(buffer, "name", search->name);
	set_parameter(buffer, "value", search->value);
	set_parameter(buffer, "type", search->type);
	set_parameter(buffer, "text", search->text);
}

void render_search_results(struct render_buffer* buffer, struct search_result_data* results, int num_results) {
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
	struct search_result_data* results = NULL;
	int num_results = 0;
	load_search_results(&results, &num_results, type, query);
	render_search_results(buffer, results, num_results);
	free(results);
}
