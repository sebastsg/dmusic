#pragma once

#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

FILE* open_file(const char* path, const char* mode);
char* read_file(const char* path, size_t* size);
char* read_file_section(const char* path, size_t* size, size_t* part_size, size_t begin, size_t end);
bool write_file(const char* path, const char* data, size_t size);
bool file_exists(const char* path);
bool directory_exists(const char* path);
bool create_directory(const char* path);
bool is_dirent_directory(const char* path, struct dirent* entry);
bool is_dirent_file(const char* path, struct dirent* entry);
