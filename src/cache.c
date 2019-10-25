#include "cache.h"
#include "database.h"
#include "config.h"
#include "files.h"
#include "system.h"
#include "type.h"
#include "generic.h"

#include <stdlib.h>
#include <string.h>

struct cached_file {
	char name[32];
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
	strcpy(file->name, name);
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
