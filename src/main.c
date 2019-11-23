#include "cache.h"
#include "config.h"
#include "database.h"
#include "files.h"
#include "format.h"
#include "install.h"
#include "network.h"
#include "stack.h"
#include "system.h"
#include "threads.h"
#include "track.h"
#include "transcode.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_resources() {
	free_network();
	free_routes();
	free_caches();
	disconnect_database();
	free_threads();
	free_stack();
	free_config();
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
			run_sql_in_directory("functions");
			run_sql_in_directory("procedures");
			seed_database();
		} else if (!strcmp(cmd, "--seed")) {
			seed_database();
		} else if (!strcmp(cmd, "--replace-functions")) {
			run_sql_in_directory("functions");
			run_sql_in_directory("procedures");
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
	initialize_threads();
	initialize_stack();
	load_config();
	connect_database();
	if (process_command(argc, argv)) {
		disconnect_database();
		free_stack();
		return 0;
	}
	initialize_routes();
	initialize_network();
	signal(SIGINT, signal_interrupt_handler);
	initialize_caches();
	while (true) {
		poll_network();
		update_threads();
	}
	free_resources();
	return 0;
}
