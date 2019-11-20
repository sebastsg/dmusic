#include "transcode.h"
#include "config.h"
#include "database.h"
#include "files.h"
#include "generic.h"
#include "stack.h"
#include "system.h"
#include "threads.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void transcode_album_release_disc(int album_release_id, int disc_num, const char* format) {
	const char* root_path = push_string(server_disc_path(album_release_id, disc_num));
	const char* dest_path = push_string_f("%s/%s", root_path, format);
	if (mkdir(dest_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
		if (errno != EEXIST) {
			print_errno_f("Failed to create directory: " A_CYAN "\"%s\"" A_RESET ".", dest_path);
			pop_string();
			pop_string();
			return;
		}
	}
	const char* allowed_formats[] = { "flac-cd-16", "flac-web-16", "flac-16", "flac-cd-24", "flac-web-24", "flac-24" };
	const size_t num_formats = sizeof(allowed_formats) / sizeof(char*);
	const char* src_path = NULL;
	DIR* dir = NULL;
	for (size_t i = 0; i < num_formats; i++) {
		src_path = push_string_f("%s/%s", root_path, allowed_formats[i]);
		if (dir = opendir(src_path)) {
			break;
		}
		pop_string();
	}
	if (!dir) {
		print_error_f("Failed to read directory: " A_CYAN "%s", src_path);
		pop_string();
		pop_string();
		pop_string();
		return;
	}
	struct dirent* entry = NULL;
	while (entry = readdir(dir)) {
		if (!is_dirent_file(src_path, entry)) {
			continue;
		}
		char* extension = strrchr(entry->d_name, '.');
		if (!extension || strcmp(extension, ".flac")) {
			continue;
		}
		const char* in = push_string_f("%s/%s", src_path, entry->d_name);
		const char* out = push_string_f("%s/%s", dest_path, entry->d_name);
		extension = strrchr(out, '.');
		if (extension) {
			*extension = '\0';
		}
		const char* ext = "mp3";
		// -q 0   = "best" quality, slower transcoding
		// -b 320 = cbr 320kbps
		// note: check out -x for when static audio is produced
		const char* command = push_string_f("lame --silent -q 0 -b 320 \"%s\" \"%s.%s\"", in, out, ext);
		system_execute(command);
		pop_string();
		pop_string();
		pop_string();
	}
	closedir(dir);
	pop_string();
	pop_string();
	pop_string();
}

void transcode_album_release(int album_release_id, const char* format) {
	if (album_release_id <= 0) {
		print_error_f("Invalid album release id: " A_CYAN "%i", album_release_id);
		return;
	}
	if (strcmp(format, "mp3-320")) {
		print_error("Can only transcode to 320 for now.");
		return;
	}
	char id_str[32];
	sprintf(id_str, "%i", album_release_id);
	const char* params[] = { id_str, format };
	insert_row("album_release_format", false, 2, params);
	const char* path = push_string(server_album_path(album_release_id));
	DIR* dir = opendir(path);
	if (!dir) {
		print_error_f("Failed to read directory: " A_CYAN "%s", path);
		pop_string();
		return;
	}
	struct dirent* entry = NULL;
	while (entry = readdir(dir)) {
		if (!is_dirent_directory(path, entry)) {
			continue;
		}
		int disc_num = atoi(entry->d_name);
		if (disc_num > 0) {
			transcode_album_release_disc(album_release_id, disc_num, format);
		}
	}
	pop_string();
	closedir(dir);
}

struct transcode_args {
	int album_release_id;
	char format[32];
};

void transcode_album_release_thread(void* data) {
	struct transcode_args* transcode = (struct transcode_args*)data;
	transcode_album_release(transcode->album_release_id, transcode->format);
	free(transcode);
}

void transcode_album_release_async(int album_release_id, const char* format) {
	struct transcode_args* transcode = (struct transcode_args*)malloc(sizeof(struct transcode_args));
	if (transcode) {
		transcode->album_release_id = album_release_id;
		snprintf(transcode->format, 32, "%s", format);
		open_thread(transcode_album_release_thread, transcode);
	}
}
