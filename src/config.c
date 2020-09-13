#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "files.h"
#include "format.h"
#include "stack.h"
#include "system.h"

#define MAX_PROPERTIES 32

struct config_property {
	char key[32];
	char* value;
	int allocated;
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
			snprintf(config[i].key, sizeof(config[i].key), "%s", key);
			config[i].allocated = strlen(value) + 1;
			config[i].value = (char*)calloc(config[i].allocated, 1);
			if (config[i].value) {
				snprintf(config[i].value, config[i].allocated, "%s", value);
			}
			break;
		}
	}
}

static void resolve_config_value(char* value, size_t size) {
	char key[512];
	char* i = strstr(value, "$(");
	while (i) {
		char* j = strchr(i, ')');
		if (!j) {
			print_error("Invalid configuration.");
			break;
		}
		int key_size = j - i - 2;
		key[key_size] = '\0';
		if (key_size < 512) {
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
	char line[512];
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

void free_config() {
	for (int i = 0; i < MAX_PROPERTIES; i++) {
		free(config[i].value);
	}
	memset(&config, 0, sizeof(config));
}

const char* server_root_path(const char* path) {
	return replace_temporary_string("%s/%s", get_property("path.root"), path);
}

const char* server_seed_path(const char* name) {
	return replace_temporary_string("%s/db/seed/%s.csv", get_property("path.root"), name);
}

const char* server_sql_path(const char* name) {
	return replace_temporary_string("%s/db/%s", get_property("path.root"), name);
}

const char* server_html_path(const char* name) {
	return replace_temporary_string("%s/html/%s.html", get_property("path.root"), name);
}

const char* server_cache_path(const char* path) {
	return replace_temporary_string("%s/cache/%s", get_property("path.cache"), path);
}

const char* server_group_path(int group_id) {
	return replace_temporary_string("%s/%i", get_property("path.groups"), group_id);
}

const char* server_album_path(int album_release_id) {
	return replace_temporary_string("%s/%i", get_property("path.albums"), album_release_id);
}

const char* server_disc_path(int album_release_id, int disc_num) {
	return replace_temporary_string("%s/%i/%i", get_property("path.albums"), album_release_id, disc_num);
}

const char* server_disc_format_path(int album_release_id, int disc_num, const char* format) {
	return replace_temporary_string("%s/%i/%i/%s", get_property("path.albums"), album_release_id, disc_num, format);
}

const char* client_group_image_path(int group_id, int num, bool thumbnail) {
	const char* option = thumbnail ? "/thumbnail" : "";
	return replace_temporary_string("/img/group/%i/%i%s", group_id, num, option);
}

const char* server_group_image_path(int group_id, int num, bool thumbnail) {
	const char* option = thumbnail ? "thumb." : "";
	const char* base = push_string(replace_temporary_string("%s/%i/%i.", get_property("path.groups"), group_id, num));
	const char* path = replace_temporary_string("%s%s%s", base, option, find_file_extension(base));
	pop_string();
	return path;
}

const char* client_album_image_path(int album_release_id, int num, bool thumbnail) {
	const char* option = thumbnail ? "/thumbnail" : "";
	return replace_temporary_string("/img/album/%i/%i%s", album_release_id, num, option);
}

const char* server_album_image_path(int album_release_id, int num, bool thumbnail) {
	const char* option = thumbnail ? "thumb." : "";
	const char* base = push_string(replace_temporary_string("%s/%i/%i.", get_property("path.albums"), album_release_id, num));
	const char* path = replace_temporary_string("%s%s%s", base, option, find_file_extension(base));
	pop_string();
	return path;
}

const char* client_uploaded_file_path(const char* filename) {
	return replace_temporary_string("/uploaded/%s", filename);
}

const char* server_uploaded_file_path(const char* filename) {
	return replace_temporary_string("%s/%s", get_property("path.uploads"), filename);
}

const char* server_uploaded_directory_file_path(const char* directory, const char* filename) {
	return replace_temporary_string("%s/%s/%s", get_property("path.uploads"), directory, filename);
}

const char* client_track_path(int album_release_id, int disc_num, int track_num) {
	return replace_temporary_string("/track/%i/%i/%i", album_release_id, disc_num, track_num);
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
	return replace_temporary_string("%s/%i/%i/%s/%i.%s", get_property("path.albums"), album_release_id, disc_num, format, track_num, ext);
}
