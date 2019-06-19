#include "config.h"
#include "format.h"
#include "files.h"

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PROPERTIES 32

struct config_property {
	char key[32];
	char value[128];
};

static struct config_property config[MAX_PROPERTIES];

static struct config_property* find_property(const char* key) {
	for (int i = 0; i < MAX_PROPERTIES; i++) {
		if (!strcmp(config[i].key, key)) {
			return &config[i];
		}
	}
	return NULL;
}

static void add_property(const char* key, const char* value) {
	for (int i = 0; i < MAX_PROPERTIES; i++) {
		if (!config[i].key[0]) {
			strcpy(config[i].key, key);
			strcpy(config[i].value, value);
			break;
		}
	}
}

static void resolve_config_value(char* value, size_t size) {
	char key[256];
	char* i = strstr(value, "$(");
	while (i) {
		char* j = strchr(i, ')');
		if (!j) {
			fprintf(stderr, "Invalid configuration.\n");
			break;
		}
		int key_size = j - i - 2;
		key[key_size] = '\0';
		if (key_size < 256) {
			memcpy(key, i + 2, key_size);
		}
		struct config_property* property = find_property(key);
		if (!property) {
			fprintf(stderr, "Configuration property %s is not defined yet.\n", key);
			break;
		}
		replace_substring(value, i, j + 1, size - (i - value), property->value);
		i = strstr(value, "$(");
	}
}

const char* get_property(const char* key) {
	struct config_property* property = find_property(key);
	if (property) {
		return property->value;
	}
	return "";
}

void load_config() {
	memset(config, 0, sizeof(config));
	char* source = read_file("/etc/dmusic/dmusic.conf", NULL);
	if (!source) {
		printf("Unable to read config file: /etc/dmusic/dmusic.conf\n");
		exit(1);
		return;
	}
	char line[256];
	const char* it = source;
	while (it = split_string(line, 256, it, '\n', NULL)) {
		trim_ends(line, " \n\t");
		if (!*line) {
			continue;
		}
		char* equals = strchr(line, '=');
		if (!equals) {
			continue;
		}
		*equals = '\0';
		char* key = line;
		char* value = equals + 1;
		trim_ends(key, " \n\t");
		trim_ends(value, " \n\t");
		if (strlen(line) == 0 || strlen(equals + 1) == 0) {
			continue;
		}
		resolve_config_value(equals + 1, 256 - (equals - line));
		add_property(key, value);
	}
	free(source);
}

char* upload_path(char* dest, size_t size, const char* path) {
	sprintf(dest, "%s/%s", get_property("path.uploads"), path);
	return dest;
}

char* seed_path(char* dest, size_t size, const char* name) {
	sprintf(dest, "%s/db/seed/%s.csv", get_property("path.root"), name);
	return dest;
}

char* sql_path(char* dest, size_t size, const char* name) {
	sprintf(dest, "%s/db/sql/%s", get_property("path.root"), name);
	return dest;
}

char* html_path(char* dest, size_t size, const char* name) {
	sprintf(dest, "%s/template/%s.html", get_property("path.root"), name);
	return dest;
}

char* cache_path(char* dest, size_t size, const char* path) {
	sprintf(dest, "%s/cache/%s", get_property("path.cache"), path);
	return dest;
}

char* group_path(char* dest, size_t size, uint64_t id) {
	sprintf(dest, "%s/%" PRIu64, get_property("path.groups"), id);
	return dest;
}

char* album_path(char* dest, size_t size, uint64_t id) {
	sprintf(dest, "%s/%" PRIu64, get_property("path.albums"), id);
	return dest;
}
