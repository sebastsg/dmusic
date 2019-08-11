#include "install.h"
#include "database.h"
#include "config.h"
#include "files.h"

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
		printf("Creating directory: %s\n", directories[i]);
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
		printf("Failed to read directory: %s\n", path);
		exit(1);
	}
	struct dirent* entry = NULL;
	while (entry = readdir(dir)) {
		if (entry->d_type == DT_REG && strstr(entry->d_name, ".sql")) {
			strcpy(path, "functions/");
			strcat(path, entry->d_name);
			printf("Executing sql file: %s\n", path);
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
	char path[512];
	char* csv = read_file(seed_path(path, 512, table), NULL);
	if (!csv) {
		printf("Seed file is missing: %s\n", path);
		return;
	}
	printf("Seeding table %s\n", table);
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
				fprintf(stderr, "Invalid number of fields in row.\n");
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
