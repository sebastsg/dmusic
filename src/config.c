#include "config.h"
#include "format.h"
#include "files.h"
#include "stack.h"
#include "system.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define MAX_PROPERTIES 32
#define MAX_THREADS    32

struct config_property {
	char key[32];
	char value[128];
};

struct config_string {
	char* value;
	size_t size;
};

static struct config_property config[MAX_PROPERTIES];
static struct config_string temporary_strings[MAX_THREADS];

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
			snprintf(config[i].key, sizeof(config[i].key), "%s",  key);
			snprintf(config[i].value, sizeof(config[i].value), "%s", value);
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
			print_error("Invalid configuration.");
			break;
		}
		int key_size = j - i - 2;
		key[key_size] = '\0';
		if (key_size < 256) {
			memcpy(key, i + 2, key_size);
		}
		struct config_property* property = find_property(key);
		if (!property) {
			print_error_f("Configuration property %s is not defined yet.", key);
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
		print_error("Unable to read config file: /etc/dmusic/dmusic.conf");
		exit(1);
		return;
	}
	char line[256];
	const char* it = source;
	while (it = split_string(line, sizeof(line), it, '\n')) {
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
		resolve_config_value(equals + 1, sizeof(line) - (equals - line));
		add_property(key, value);
	}
	free(source);
}

static const char* write_temporary_string(const char* format, ...) {
	const int index = 0;
	struct config_string* string = &temporary_strings[index];
	if (!string->value) {
		string->value = (char*)malloc(32);
		if (string->value) {
			string->size = 32;
		} else {
			return "";
		}
	}
	va_list args;
	va_start(args, format);
	int size = vsnprintf(string->value, string->size, format, args);
	va_end(args);
	if (size >= (int)string->size) {
		char* new_value = (char*)realloc(string->value, size * 2);
		if (new_value) {
			string->value = new_value;
			string->size = size * 2;
			va_start(args, format);
			vsnprintf(string->value, string->size, format, args);
			va_end(args);
		} else {
			free(string->value);
			string->value = NULL;
			string->size = 0;
		}
	}
	return string->value ? string->value : "";
}

const char* server_root_path(const char* path) {
	return write_temporary_string("%s/%s", get_property("path.root"), path);
}

const char* server_session_path(const char* id) {
	return write_temporary_string("%s/sessions/%s", get_property("path.root"), id);
}

const char* server_seed_path(const char* name) {
	return write_temporary_string("%s/db/seed/%s.csv", get_property("path.root"), name);
}

const char* server_sql_path(const char* name) {
	return write_temporary_string("%s/db/%s", get_property("path.root"), name);
}

const char* server_html_path(const char* name) {
	return write_temporary_string("%s/html/%s.html", get_property("path.root"), name);
}

const char* server_cache_path(const char* path) {
	return write_temporary_string("%s/cache/%s", get_property("path.cache"), path);
}

const char* server_group_path(int group_id) {
	return write_temporary_string("%s/%i", get_property("path.groups"), group_id);
}

const char* server_album_path(int album_release_id) {
	return write_temporary_string("%s/%i", get_property("path.albums"), album_release_id);
}

const char* server_disc_path(int album_release_id, int disc_num) {
	return write_temporary_string("%s/%i/%i", get_property("path.albums"), album_release_id, disc_num);
}

const char* server_disc_format_path(int album_release_id, int disc_num, const char* format) {
	return write_temporary_string("%s/%i/%i/%s", get_property("path.albums"), album_release_id, disc_num, format);
}

const char* client_group_image_path(int group_id, int num) {
	return write_temporary_string("/img/group/%i/%i", group_id, num);
}

const char* server_group_image_path(int group_id, int num) {
	const char* base = push_string(write_temporary_string("%s/%i/%i.", get_property("path.groups"), group_id, num));
	const char* path = write_temporary_string("%s%s", base, find_file_extension(base));
	pop_string();
	return path;
}

const char* client_album_image_path(int album_release_id, int num) {
	return write_temporary_string("/img/album/%i/%i", album_release_id, num);
}

const char* server_album_image_path(int album_release_id, int num) {
	const char* base = push_string(write_temporary_string("%s/%i/%i.", get_property("path.albums"), album_release_id, num));
	const char* path = write_temporary_string("%s%s", base, find_file_extension(base));
	pop_string();
	return path;
}

const char* client_uploaded_file_path(const char* filename) {
	return write_temporary_string("/uploaded/%s", filename);
}

const char* server_uploaded_file_path(const char* filename) {
	return write_temporary_string("%s/%s", get_property("path.uploads"), filename);
}

const char* server_uploaded_directory_file_path(const char* directory, const char* filename) {
	return write_temporary_string("%s/%s/%s", get_property("path.uploads"), directory, filename);
}

const char* client_track_path(int album_release_id, int disc_num, int track_num) {
	return write_temporary_string("/track/%i/%i/%i", album_release_id, disc_num, track_num);
}

const char* server_track_path(const char* format, int album_release_id, int disc_num, int track_num) {
	char ext[16];
	if (!strncmp(format, "mp3", 3)) {
		strcpy(ext, "mp3");
	} else if (!strncmp(format, "aac", 3)) {
		strcpy(ext, "m4a");
	} else if (!strncmp(format, "flac", 4)) {
		strcpy(ext, "flac");
	} else {
		return "";
	}
	return write_temporary_string("%s/%i/%i/%s/%i.%s", get_property("path.albums"), album_release_id, disc_num, format, track_num, ext);
}
