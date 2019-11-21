#include "install.h"
#include "config.h"
#include "database.h"
#include "files.h"
#include "stack.h"
#include "system.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void create_directories() {
	const char* directories[] = {
		get_property("path.cache"),
		get_property("path.data"),
		get_property("path.uploads"),
		get_property("path.groups"),
		get_property("path.albums"),
		get_property("path.videos")
	};
	const size_t num_directories = sizeof(directories) / sizeof(const char*);
	for (size_t i = 0; i < num_directories; i++) {
		print_info_f("Creating directory: %s", directories[i]);
		create_directory(directories[i]);
	}
}

void install_database() {
	execute_sql_file("create_database.sql", true);
}

void run_sql_in_directory(const char* directory) {
	const char* path = push_string_f("%s/db/%s", get_property("path.root"), directory);
	DIR* dir = opendir(path);
	if (!dir) {
		print_error_f("Failed to read directory: %s", path);
		exit(1);
	}
	struct dirent* entry = NULL;
	while (entry = readdir(dir)) {
		if (is_dirent_file(path, entry) && strstr(entry->d_name, ".sql")) {
			const char* sql_file = push_string_f("%s/%s", directory, entry->d_name);
			print_info_f("Executing sql file: %s", sql_file);
			execute_sql_file(sql_file, false);
			pop_string();
		}
	}
	closedir(dir);
	pop_string();
}

static int csv_num_fields(const char* csv) {
	int fields = 0;
	const char* it = csv;
	const char* line = strchr(csv, '\n');
	do {
		fields++;
		it = strchr(it + 1, ';');
	} while (it && line > it);
	return fields;
}

static void seed_table(const char* table) {
	const char* path = server_seed_path(table);
	char* csv = read_file(path, NULL);
	if (!csv) {
		print_error_f("Seed file is missing: %s", path);
		return;
	}
	print_info_f("Seeding table %s", table);
	char* row = csv;
	int fields = csv_num_fields(csv);
	while (*row) {
		char* end = NULL;
		const char* params[16];
		for (int i = 0; i < fields - 1; i++) {
			end = strchr(row, ';');
			if (end) {
				*end = '\0';
				params[i] = row;
				row = end + 1;
			} else {
				print_error("Invalid number of fields in row.");
			}
		}
		end = strchr(row, '\n');
		params[fields - 1] = row;
		if (end) {
			if (*(end - 1) == '\r') {
				*(end - 1) = '\0';
			}
			*end = '\0';
			row = end + 1;
		}
		insert_row(table, false, fields, params);
		if (!end) {
			break;
		}
	}
	free(csv);
}

void seed_database() {
	seed_table("country");
	seed_table("language");
	seed_table("role");
	seed_table("album_type");
	seed_table("album_release_type");
	seed_table("tag");
	seed_table("audio_format");
}
