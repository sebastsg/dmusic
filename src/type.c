#include "type.h"
#include "database.h"
#include "cache.h"
#include "render.h"

#include <stdlib.h>
#include <string.h>

void load_options(struct select_options* options, const char* type) {
	char query[128];
	snprintf(query, sizeof(query), "select * from %s order by \"name\"", type);
	PGresult* result = execute_sql(query, NULL, 0);
	options->count = PQntuples(result);
	options->options = (struct select_option*)malloc(options->count * sizeof(struct select_option));
	if (options->options) {
		for (int i = 0; i < options->count; i++) {
			struct select_option* option = &options->options[i];
			strcpy(option->code, PQgetvalue(result, i, 0));
			strcpy(option->name, PQgetvalue(result, i, 1));
		}
	}
	PQclear(result);
}

void render_select_option(struct render_buffer* buffer, const char* key, const char* value, const char* text, bool selected) {
	set_parameter(buffer, key, get_cached_file("html/select_option.html", NULL));
	set_parameter(buffer, "value", value);
	set_parameter(buffer, "selected", selected ? "selected" : "");
	set_parameter(buffer, "text", text);
}
