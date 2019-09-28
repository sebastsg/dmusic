#include "config.h"
#include "format.h"
#include "install.h"
#include "database.h"
#include "network.h"
#include "cache.h"
#include "transcode.h"
#include "files.h"
#include "stack.h"
#include "system.h"
#include "track.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static void free_resources() {
	free_network();
	free_file_cache();
	free_options_cache();
	disconnect_database();
	free_stack();
}

static void signal_interrupt_handler(int signal_num) {
	print_info("Closing dmusic");
	free_resources();
	exit(0);
}

static bool process_command(int argc, char** argv) {
	char* cmd = argc > 1 ? argv[1] : NULL;
	if (cmd) {
		if (!strcmp(cmd, "--install")) {
			create_directories();
			install_database();
			replace_database_functions();
			seed_database();
		} else if (!strcmp(cmd, "--seed")) {
			seed_database();
		} else if (!strcmp(cmd, "--replace-functions")) {
			replace_database_functions();
		} else if (!strcmp(cmd, "--update-track-durations")) {
			update_all_track_durations();
		} else if (!strcmp(cmd, "--transcode")) {
			if (argc > 2) {
				transcode_album_release(atoi(argv[2]), argc > 3 ? argv[3] : "mp3-320");
			}
		}
		return true;
	}
	return false;
}

int main(int argc, char** argv) {
	srand(time(NULL));
	initialize_stack();
	load_config();
	connect_database();
	if (process_command(argc, argv)) {
		disconnect_database();
		free_stack();
		return 0;
	}
	initialize_network();
	signal(SIGINT, signal_interrupt_handler);
	load_file_cache();
	load_options_cache();
	while (true) {
		poll_network();
	}
	free_resources();
	return 0;
}
