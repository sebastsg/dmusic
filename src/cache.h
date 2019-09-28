#pragma once

#include <stddef.h>

struct select_options;

void load_file_cache();
void free_file_cache();
char* get_cached_file(const char* name, size_t* size);

void load_options_cache();
void free_options_cache();
const struct select_options* get_cached_options(const char* name);
