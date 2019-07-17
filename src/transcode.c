#include "transcode.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

#if PLATFORM_IS_LINUX
#include <unistd.h>
#endif

static void transcode_album_release_disc(int album_release_id, int disc_num, const char* format) {
	char root_path[512];
	server_disc_path(root_path, album_release_id, disc_num);
	char dest_path[1024];
	sprintf(dest_path, "%s/%s", root_path, format);
	if (mkdir(dest_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
		if (errno != EEXIST) {
			fprintf(stderr, "Failed to create directory: \"%s\". Error: %s\n", dest_path, strerror(errno));
			return;
		}
	}
	char src_path[1024];
	sprintf(src_path, "%s/flac-cd-16", root_path);
	DIR* dir = opendir(src_path);
	if (!dir) {
		sprintf(src_path, "%s/flac-web-16", root_path);
		dir = opendir(src_path);
		if (!dir) {
			fprintf(stderr, "Failed to read directory: %s\n", src_path);
			return;
		}
	}
	struct dirent* entry = NULL;
	while (entry = readdir(dir)) {
		if (entry->d_type != DT_REG) {
			continue;
		}
		char* extension = strrchr(entry->d_name, '.');
		if (!extension || strcmp(extension, ".flac")) {
			continue;
		}
		char in[2048];
		sprintf(in, "%s/%s", src_path, entry->d_name);
		char out[2048];
		sprintf(out, "%s/%s", dest_path, entry->d_name);
		extension = strrchr(out, '.');
		if (extension) {
			*(extension + 1) = '\0';
		}
		strcat(out, "mp3");
		// -S     = silent mode
		// -q 0   = "best" quality, slower transcoding - not using for now... but worth keeping in mind
		// -b 320 = cbr 320kbps
		// note: check out -x for when static audio is produced
		char command[4800];
		sprintf(command, "lame -S -b 320 \"%s\" \"%s\"", in, out);
		printf("%s\n", command);
		system(command);
	}
	closedir(dir);
}

void transcode_album_release(int album_release_id, const char* format) {
	if (album_release_id <= 0) {
		fprintf(stderr, "Invalid album release id: %i\n", album_release_id);
		return;
	}
	if (strcmp(format, "mp3-320")) {
		fprintf(stderr, "Can only transcode to 320 for now.\n");
		return;
	}
	char path[512];
	album_path(path, 512, album_release_id);
	DIR* dir = opendir(path);
	struct dirent* entry = NULL;
	while (entry = readdir(dir)) {
		if (entry->d_type != DT_DIR) {
			continue;
		}
		int disc_num = atoi(entry->d_name);
		if (disc_num > 0) {
			transcode_album_release_disc(album_release_id, disc_num, format);
		}
	}
	closedir(dir);
}
