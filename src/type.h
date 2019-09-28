#pragma once

#include <stdbool.h>

struct render_buffer;

struct select_option {
	char code[64];
	char name[128];
};

struct select_options {
	struct select_option* options;
	int count;
};

void load_options(struct select_options* options, const char* type);
void render_select_option(struct render_buffer* buffer, const char* key, const char* value, const char* text, bool selected);
