#include "config.h"
#include "format.h"
#include "data.h"
#include "install.h"
#include "database.h"
#include "network.h"
#include "cache.h"

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

int main(int argc, char** argv) {
	srand(time(NULL));
	load_config();
	connect_database();
	char* cmd = argc > 1 ? argv[1] : NULL;
	if (cmd) {
		if (!strcmp(cmd, "install")) {
			create_directories();
			install_database();
			disconnect_database();
		} else if (!strcmp(cmd, "seed")) {
			seed_database();
			disconnect_database();
		}
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
