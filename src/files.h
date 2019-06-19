#pragma once

#include <stddef.h>
#include <stdbool.h>

char* read_file(const char* path, size_t* size);
bool write_file(const char* path, const char* data, size_t size);
bool file_exists(const char* path);
