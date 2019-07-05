#include "config.h"
#include "format.h"
#include "data.h"
#include "cache.h"
#include "install.h"
#include "database.h"
#include "transcode.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void cmd_render(char** args, int count) {
	char* html = render(args, count);
	if (html) {
		printf("%s", html);
		free(html);
	}
}

void cmd_path(char** args, int count) {
	if (count == 0) {
		printf("%s", get_property("path.root"));
		return;
	}
	if (!strcmp(args[0], "data")) {
		printf("%s", get_property("path.data_root"));
	} else if (!strcmp(args[0], "uploads")) {
		printf("%s", get_property("path.uploads"));
	} else if (!strcmp(args[0], "groups")) {
		printf("%s", get_property("path.groups"));
	} else if (!strcmp(args[0], "albums")) {
		printf("%s", get_property("path.albums"));
	} else if (!strcmp(args[0], "videos")) {
		printf("%s", get_property("path.videos"));
	} else {
		printf("unknown");
	}
}

void cmd_create(char** args, int count) {
	if (count > 1) {
		char* table = args[0];
		args++;
		count--;
		bool has_serial_id = !strcmp(args[0], "true");
		args++;
		count--;
		if (!strcmp(table, "session_track")) {
			printf("%i", create_session_track(args[0], atoi(args[1]), atoi(args[2]), atoi(args[3])));
		} else {
			printf("%i", insert_row(table, has_serial_id, count, (const char**)args));
		}
	}
}

void cmd_transcode(char** args, int count) {
	if (count >= 2) {
		transcode_album_release(atoi(args[0]), args[1]);
	}
}

void cmd_register(char** args, int count) {
	if (count >= 2) {
		bool success = register_user(args[0], args[1]);
		puts(success ? "1" : "0");
	}
}

void cmd_login(char** args, int count) {
	if (count >= 2) {
		bool success = login_user(args[0], args[1]);
		puts(success ? "1" : "0");
	}
}

int main(int argc, char** argv) {
	if (argc <= 1) {
		fprintf(stderr, "No command specified.\n");
		return -1;
	}
	char* cmd = argv[1];
	char** args = argv + 2;
	int count = argc - 2;

	srand(time(NULL));
	load_config();
	connect_database();
	
	if (!strcmp(cmd, "install")) {
		create_directories();
		install_database();
		disconnect_database();
		return 0;
	}
	
	load_memory_cache();

	if (!strcmp(cmd, "render")) {
		cmd_render(args, count);
	} else if(!strcmp(cmd, "path")) {
		cmd_path(args, count);
	} else if (!strcmp(cmd, "create")) {
		cmd_create(args, count);
	} else if (!strcmp(cmd, "transcode")) {
		cmd_transcode(args, count);
	} else if (!strcmp(cmd, "seed")) {
		seed_database();
	} else if (!strcmp(cmd, "register")) {
		cmd_register(args, count);
	} else if (!strcmp(cmd, "login")) {
		cmd_login(args, count);
	} else {
		fprintf(stderr, "Invalid command: %s\n", cmd);
	}

	disconnect_database();
	return 0;
}
