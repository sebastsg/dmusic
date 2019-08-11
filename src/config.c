#include "config.h"
#include "format.h"
#include "files.h"

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
	while (it = split_string(line, 256, it, '\n')) {
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

char* root_path(char* dest, size_t size, const char* path) {
	sprintf(dest, "%s/%s", get_property("path.root"), path);
	return dest;
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
	sprintf(dest, "%s/db/%s", get_property("path.root"), name);
	return dest;
}

char* html_path(char* dest, size_t size, const char* name) {
	sprintf(dest, "%s/html/%s.html", get_property("path.root"), name);
	return dest;
}

char* cache_path(char* dest, size_t size, const char* path) {
	sprintf(dest, "%s/cache/%s", get_property("path.cache"), path);
	return dest;
}

char* group_path(char* dest, size_t size, int group_id) {
	sprintf(dest, "%s/%i", get_property("path.groups"), group_id);
	return dest;
}

char* album_path(char* dest, size_t size, int album_release_id) {
	sprintf(dest, "%s/%i", get_property("path.albums"), album_release_id);
	return dest;
}

char* server_disc_path(char* dest, int album_release_id, int disc_num) {
	sprintf(dest, "%s/%i/%i", get_property("path.albums"), album_release_id, disc_num);
	return dest;
}

char* server_disc_format_path(char* dest, int album_release_id, int disc_num, const char* format) {
	sprintf(dest, "%s/%i/%i/%s", get_property("path.albums"), album_release_id, disc_num, format);
	return dest;
}

char* client_group_image_path(char* dest, int group_id, int num) {
	sprintf(dest, "/img/group/%i/%i", group_id, num);
	return dest;
}

char* server_group_image_path(char* dest, int group_id, int num) {
	sprintf(dest, "%s/%i/%i.", get_property("path.groups"), group_id, num);
	strcat(dest, find_file_extension(dest));
	return dest;
}

char* client_album_image_path(char* dest, int album_release_id, int num) {
	sprintf(dest, "/img/album/%i/%i", album_release_id, num);
	return dest;
}

char* server_album_image_path(char* dest, int album_release_id, int num) {
	sprintf(dest, "%s/%i/%i.", get_property("path.albums"), album_release_id, num);
	strcat(dest, find_file_extension(dest));
	return dest;
}

char* client_uploaded_file_path(char* dest, const char* filename) {
	sprintf(dest, "/uploaded/%s", filename);
	return dest;
}

char* server_uploaded_file_path(char* dest, const char* filename) {
	sprintf(dest, "%s/%s", get_property("path.uploads"), filename);
	return dest;
}

char* client_track_path(char* dest, int album_release_id, int disc_num, int track_num) {
	sprintf(dest, "/track/%i/%i/%i", album_release_id, disc_num, track_num);
	return dest;
}

char* server_track_path(char* dest, const char* format, int album_release_id, int disc_num, int track_num) {
	char ext[16];
	if (!strncmp(format, "mp3", 3)) {
		strcpy(ext, "mp3");
	} else if (!strncmp(format, "aac", 3)) {
		strcpy(ext, "m4a");
	} else if (!strncmp(format, "flac", 4)) {
		strcpy(ext, "flac");
	} else {
		return dest;
	}
	sprintf(dest, "%s/%i/%i/%s/%i.%s", get_property("path.albums"), album_release_id, disc_num, format, track_num, ext);
	return dest;
}
