#pragma once

#include <stddef.h>

#ifdef __GNUC__
# define COMPILER_IS_GCC 1
#else
# define COMPILER_IS_GCC 0
#endif

#ifdef __linux__
# define PLATFORM_IS_LINUX 1
#else
# define PLATFORM_IS_LINUX 0
#endif

void load_config();
const char* get_property(const char* key);

char* root_path(char* dest, size_t size, const char* path);
char* upload_path(char* dest, size_t size, const char* path);
char* seed_path(char* dest, size_t size, const char* name);
char* sql_path(char* dest, size_t size, const char* name);
char* html_path(char* dest, size_t size, const char* name);
char* cache_path(char* dest, size_t size, const char* path);
char* group_path(char* dest, size_t size, int group_id);
char* album_path(char* dest, size_t size, int album_release_id);
char* album_disc_path(char* dest, size_t size, int album_release_id, int disc_num);
