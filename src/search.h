#pragma once

struct render_buffer;

struct search_data {
	char name[64];
	char type[32];
	char value[128];
	char text[128];
};

struct search_result_data {
	char value[128];
	char text[128];
};

void load_search_results(struct search_result_data** results, int* count, const char* type, const char* query);
void render_search(struct render_buffer* buffer, const char* key, struct search_data* search);
void render_search_query(struct render_buffer* buffer, const char* type, const char* query);
