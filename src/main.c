#include "config.h"
#include "format.h"
#include "data.h"
#include "cache.h"
#include "install.h"
#include "database.h"

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
		bool has_serial_id = (strcmp(args[0], "true"));
		args++;
		count--;
		printf("%i", (int)insert_row(table, has_serial_id, count, (const char**)args));
	}
}

void cmd_transcode(char** args, int count) {
	if (count < 2) {
		fprintf(stderr, "Invalid arguments");
		return;
	}
	uint64_t album_release_id = atoi(args[0]);
	if (album_release_id == 0) {
		fprintf(stderr, "First argument must be an album release id.");
		return;
	}
	/*std::string format = args[1];
	if (format != "mp3-320") {
		fprintf(stderr, "Can only transcode to 320 for now.");
		return;
	}
	char buffer[512];
	auto files = std::filesystem::recursive_directory_iterator(album_path(buffer, 512, album_release_id));
	for (auto& file : files) {
		if (std::filesystem::is_directory(file)) {
			continue;
		}
		auto path = file.path();
		if (path.extension() != ".flac") {
			continue;
		}
		std::string in = path.string();
		std::string out_dir = path.parent_path().string() + "/../" + format;
		std::filesystem::create_directory(out_dir);
		std::string out = out_dir + "/" + path.stem().string() + ".mp3";
		// -S     = silent mode
		// -q 0   = "best" quality, slower transcoding - not using for now... but worth keeping in mind
		// -b 320 = cbr 320kbps
		// note: check out -x for when static audio is produced
		char command[1024];
		sprintf(command, "lame -S -b 320 \"%s\" \"%s\"", in.c_str(), out.c_str());
		printf("Command: %s\n", command);
		system(command);
	}*/
}

int main(int argc, char** argv) {
	if (argc <= 1) {
		fprintf(stderr, "No command specified.\n");
		return -1;
	}
	load_config();
	connect_database();
	load_memory_cache();

	char* cmd = argv[1];
	char** args = argv + 2;
	int count = argc - 2;

	if (!strcmp(cmd, "render")) {
		cmd_render(args, count);
	} else if(!strcmp(cmd, "path")) {
		cmd_path(args, count);
	} else if (!strcmp(cmd, "create")) {
		cmd_create(args, count);
	} else if (!strcmp(cmd, "transcode")) {
		cmd_transcode(args, count);
	} else if (!strcmp(cmd, "install")) {
		create_directories();
		install_database();
	} else if (!strcmp(cmd, "seed")) {
		seed_database();
	} else {
		fprintf(stderr, "Invalid command: %s\n", cmd);
	}

	disconnect_database();
	return 0;
}
