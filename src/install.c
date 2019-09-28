#include "install.h"
#include "database.h"
#include "config.h"
#include "files.h"
#include "system.h"

#include <stdbool.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

void create_directories() {
	const char* directories[] = {
		get_property("path.data"),
		get_property("path.uploads"),
		get_property("path.groups"),
		get_property("path.albums"),
		get_property("path.videos")
	};
	for (int i = 0; i < 5; i++) {
		print_info_f("Creating directory: %s", directories[i]);
		create_directory(directories[i]);
	}
}

void install_database() {
	execute_sql_file("create_database.sql", true);
}

void replace_database_functions() {
	char path[512];
	sprintf(path, "%s/db/functions", get_property("path.root"));
	DIR* dir = opendir(path);
	if (!dir) {
		print_error_f("Failed to read directory: %s", path);
		exit(1);
	}
	struct dirent* entry = NULL;
	while (entry = readdir(dir)) {
		if (is_dirent_file(path, entry) && strstr(entry->d_name, ".sql")) {
			strcpy(path, "functions/");
			strcat(path, entry->d_name);
			print_info_f("Executing sql file: %s", path);
			execute_sql_file(path, false);
		}
	}
	closedir(dir);
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
