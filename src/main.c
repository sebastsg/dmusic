#include "config.h"
#include "format.h"
#include "data.h"
#include "cache.h"
#include "install.h"
#include "database.h"
#include "transcode.h"
#include "network.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static void free_resources() {
	free_network();
	disconnect_database();
}

static void signal_interrupt_handler(int signal_num) {
	puts("Closing dmusic\n");
	free_resources();
	exit(0);
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
	srand(time(NULL));
	load_config();
	connect_database();
	char* cmd = argc > 1 ? argv[1] : NULL;
	if (cmd && !strcmp(cmd, "install")) {
		create_directories();
		install_database();
		disconnect_database();
		return 0;
	}
	initialize_network();
	signal(SIGINT, signal_interrupt_handler);
	load_memory_cache();
	while (true) {
		poll_network();
	}
	free_resources();
	return 0;
}
