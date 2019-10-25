#pragma once

#include <stddef.h>

struct select_options;

void initialize_caches();
void free_caches();
char* get_cached_file(const char* name, size_t* size);
const struct select_options* get_cached_options(const char* name);
