#include "cache.h"
#include "config.h"
#include "database.h"
#include "files.h"
#include "generic.h"
#include "system.h"
#include "type.h"

#include <stdlib.h>
#include <string.h>

struct cached_file {
	char name[64];
	char* body;
	size_t size;
};

struct file_cache {
	struct cached_file* files;
	int count;
	int allocated;
};

struct cached_options {
	char name[32];
	struct select_options options;
};

struct options_cache {
	struct cached_options* types;
	int count;
	int allocated;
};

static struct file_cache files;
static struct options_cache options;

struct country_cache {
	char* codes; // 4 bytes per code: no\0\0dk\0\0us\0\0
	char* names; // 96 bytes per name
	int count;
	int size_of_codes;
	int size_of_names;
};

static struct country_cache countries;

static void load_countries() {
	PGresult* result = execute_sql("select \"code\", \"name\" from \"country\"", NULL, 0);
	countries.count = PQntuples(result);
	countries.codes = (char*)calloc(countries.count, 4);
	countries.names = (char*)calloc(countries.count, 96);
	if (countries.codes && countries.names) {
		for (int i = 0; i < countries.count; i++) {
			strcpy(countries.codes + i * 4, PQgetvalue(result, i, 0));
			strcpy(countries.names + i * 96, PQgetvalue(result, i, 1));
		}
		countries.size_of_codes = countries.count * 4;
		countries.size_of_names = countries.count * 96;
	}
	PQclear(result);
}

static void free_countries() {
	free(countries.codes);
	free(countries.names);
	memset(&countries, 0, sizeof(struct country_cache));
}

const char* get_country_name(const char* code) {
	for (int i = 0; i < countries.size_of_codes; i += 4) {
		if (countries.codes[i] == code[0] && countries.codes[i + 1] == code[1]) {
			return countries.names + i * 24;
		}
	}
	return code;
}

char* cache_file(struct file_cache* cache, const char* name, size_t* size) {
	enum resize_status status = resize_array((void**)&cache->files, sizeof(struct cached_file), &cache->allocated, cache->count + 1);
	if (status == RESIZE_FAILED) {
		print_error_f("Failed to allocate %i cached files.", files.allocated);
		return NULL;
	}
	if (status == RESIZE_DONE) {
		print_info_f("Allocated %i cached files.", files.allocated);
	}
	struct cached_file* file = &cache->files[cache->count];
	snprintf(file->name, sizeof(file->name), "%s", name);
	const char* path = server_root_path(name);
	file->body = read_file(path, &file->size);
	if (file->body) {
		print_info_f("Cached file: %s", path);
	} else {
		print_error_f("Failed to load file: %s", path);
	}
	if (size) {
		*size = file->size;
	}
	cache->count++;
	return file->body;
}

void initialize_caches() {
	memset(&files, 0, sizeof(files));
	memset(&options, 0, sizeof(options));
	load_countries();
}

void free_caches() {
	for (int i = 0; i < files.count; i++) {
		free(files.files[i].body);
	}
	free(files.files);
	memset(&files, 0, sizeof(files));
	for (int i = 0; i < options.count; i++) {
		free(options.types[i].options.options);
	}
	free(options.types);
	memset(&options, 0, sizeof(options));
	free_countries();
}

char* get_cached_file(const char* name, size_t* size) {
	for (int i = 0; i < files.count; i++) {
		if (!strcmp(files.files[i].name, name)) {
			if (size) {
				*size = files.files[i].size;
			}
			return files.files[i].body;
		}
	}
	return cache_file(&files, name, size);
}

const struct select_options* get_cached_options(const char* name) {
	for (int i = 0; i < options.count; i++) {
		if (!strcmp(options.types[i].name, name)) {
			return &options.types[i].options;
		}
	}
	enum resize_status status = resize_array((void**)&options.types, sizeof(struct cached_options), &options.allocated, options.count + 1);
	if (status == RESIZE_FAILED) {
		print_error_f("Failed to allocate %i cached option types.", options.allocated);
		return NULL;
	} else if (status == RESIZE_DONE) {
		print_info_f("Allocated %i cached option types.", options.allocated);
	}
	strcpy(options.types[options.count].name, name);
	load_options(&options.types[options.count].options, name);
	print_info_f("Cached %s options", name);
	options.count++;
	return &options.types[options.count - 1].options;
}
