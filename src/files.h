#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <dirent.h>

char* read_file(const char* path, size_t* size);
bool write_file(const char* path, const char* data, size_t size);
bool file_exists(const char* path);
bool directory_exists(const char* path);
bool create_directory(const char* path);
bool is_dirent_directory(const char* path, struct dirent* entry);
bool is_dirent_file(const char* path, struct dirent* entry);
