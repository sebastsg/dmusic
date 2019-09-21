#include "data.h"
#include "prepare.h"
#include "format.h"
#include "config.h"
#include "files.h"
#include "database.h"
#include "stack.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <ftw.h>
#include <sys/stat.h>
#include <unistd.h>

char* system_output(const char* command, size_t* out_size) {
	FILE* process = popen(command, "r");
	if (!process) {
		fprintf(stderr, "Failed to run \"%s\". Error: %s\n", command, strerror(errno));
		return NULL;
	}
	size_t index = 0;
	size_t allocated_size = 512;
	char* buffer = (char*)malloc(allocated_size);
	*buffer = '\0';
	size_t last_read_size = 0;
	while (last_read_size = fread(buffer + index, 1, allocated_size - index, process)) {
		index += last_read_size;
		if (index + 1 >= allocated_size) {
			char* new_buffer = (char*)realloc(buffer, allocated_size * 2);
			if (!new_buffer) {
				break;
			}
			buffer = new_buffer;
			allocated_size *= 2;
		}
	}
	pclose(process);
	if (out_size) {
		*out_size = index;
	}
	buffer[index] = '\0';
	return buffer;
}

void load_prepare_attachment(struct prepare_attachment_data* attachment, const char* path) {
	const char* name = strrchr(path, '/') + 1;
	attachment->name = copy_string(name);
	attachment->path = copy_string(path);
	attachment->link = copy_string(client_uploaded_file_path(path));
	if (is_extension_image(path)) {
		attachment->preview = copy_string(attachment->link);
	}
	attachment->selected_target = guess_targets(name, &attachment->targets);
}

struct group_guess_search {
	struct search_result_data search;
	int count;
};

static void guess_group_name(char* value, char* text, const char* filename) {
	struct group_guess_search* searches = (struct group_guess_search*)malloc(sizeof(struct group_guess_search) * 100);
	if (!searches) {
		return;
	}
	int num_searches = 0;
	char word[256];
	const char* filename_it = filename;
	while (filename_it = split_string(word, 256, filename_it, ' ')) {
		trim_ends(word, " \t");
		struct search_result_data* group_searches = NULL;
		int group_search_count = 0;
		load_search_results(&group_searches, &group_search_count, "groups", word);
		for (int i = 0; i < group_search_count; i++) {
			bool add = true;
			for (int j = 0; j < num_searches; j++) {
				if (strcmp(searches[j].search.value, group_searches[i].value) == 0) {
					searches[j].count++;
					add = false;
					break;
				}
			}
			if (add) {
				searches[num_searches].search = group_searches[i];
				searches[num_searches].count = 1;
				num_searches++;
				// todo: resize
			}
		}
		free(group_searches);
	}
	if (num_searches == 0) {
		free(searches);
		return;
	}
	int best = 0;
	for (int i = 0; i < num_searches; i++) {
		if (searches[i].count > searches[best].count) {
			best = i;
		}
	}
	strcpy(value, searches[best].search.value);
	strcpy(text, searches[best].search.text);
	free(searches);
}

static void guess_album_type(char* type, int num_tracks) {
	if (num_tracks < 4) {
		strcpy(type, "single");
	} else if (num_tracks < 8) {
		strcpy(type, "ep");
	} else {
		strcpy(type, "album");
	}
}

static void guess_audio_format(char* format, const char* filename) {
	if (strstr(filename, "WEB FLAC")) {
		strcpy(format, "flac-web");
	} else if (strstr(filename, "MP3")) {
		strcpy(format, "mp3-320");
	} else {
		strcpy(format, "flac");
	}
}

static void* nftw_user_data_1 = NULL;
static void* nftw_user_data_2 = NULL;

/*
 * @nftw_user_data_1: struct prepare_attachment_data* attachments
 * @nftw_user_data_2: int* num_attachments
 */
static int nftw_guess_attachment(const char* path, const struct stat* stat_buffer, int flag, struct FTW* ftw_buffer) {
	if (flag != FTW_F) {
		printf("Skipping invalid entry. Not a file: %s\n", path);
		return 0;
	}
	if (is_extension_audio(strrchr(path, '.'))) {
		printf("Skipping invalid attachment file. Is audio: %s\n", path);
		return 0;
	}
	struct prepare_attachment_data* attachments = (struct prepare_attachment_data*)nftw_user_data_1;
	int* num_attachments = (int*)nftw_user_data_2;
	size_t uploads_path_size = strlen(get_property("path.uploads"));
	const char* relative_path = (strlen(path) > uploads_path_size ? path + uploads_path_size : path);
	load_prepare_attachment(&attachments[*num_attachments], relative_path);
	(*num_attachments)++;
	return 0;
}

static void guess_attachments(struct prepare_attachment_data* attachments, int* num_attachments, const char* path) {
	*num_attachments = 0;
	const char* full_path = push_string(server_uploaded_file_path(path));
	nftw_user_data_1 = attachments;
	nftw_user_data_2 = num_attachments;
	if (nftw(full_path, nftw_guess_attachment, 20, 0) == -1) {
		print_error_f("\"%s\" error occured while preparing attachments: %s\n", strerror(errno), full_path);
	}
	pop_string();
}

/*
 * @nftw_user_data_1: struct prepare_disc_data* discs
 * @nftw_user_data_2: int* num_discs
 */
static int nftw_guess_track(const char* path, const struct stat* stat_buffer, int flag, struct FTW* ftw_buffer) {
	if (flag != FTW_F) {
		printf("Skipping invalid entry. Not a file: %s\n", path);
		return 0;
	}
	if (!is_extension_audio(strrchr(path, '.'))) {
		printf("Skipping invalid track file. Not audio: %s\n", path);
		return 0;
	}
	printf("Found track file: %s\n", path);
	struct prepare_disc_data* discs = (struct prepare_disc_data*)nftw_user_data_1;
	int* num_discs = (int*)nftw_user_data_2;
	size_t uploads_path_size = strlen(get_property("path.uploads"));
	const char* uploaded_name = strrchr(path, '/') + 1;
	int disc_num = guess_disc_num(uploaded_name);
	if (disc_num > 5) {
		// TODO: Compare with number of tracks? If there are 100 audio files, perhaps it is right.
		print_error_f("The filenames make it seem like there are %i discs. This is unlikely, and currently not allowed.", disc_num);
		return 0;
	}
	while (disc_num > *num_discs || *num_discs == 0) {
		discs[*num_discs].num = *num_discs + 1;
		discs[*num_discs].num_tracks = 0;
		(*num_discs)++;
	}
	struct prepare_disc_data* disc = &discs[disc_num - 1];
	struct prepare_track_data* track = &disc->tracks[disc->num_tracks];
	disc->num_tracks++;
	track->num = guess_track_num(uploaded_name, true);
	guess_track_name(track->fixed, uploaded_name);
	strcpy(track->uploaded, uploaded_name);
	strcpy(track->extension, strrchr(uploaded_name, '.') + 1);
	strcpy(track->path, path + uploads_path_size);
	return 0;
}

static int sort_prepare_track_num(const void* a, const void* b) {
	return ((struct prepare_track_data*)a)->num - ((struct prepare_track_data*)b)->num;
}

static void guess_tracks(struct prepare_disc_data* discs, int* num_discs, const char* path) {
	*num_discs = 0;
	const char* full_path = push_string(server_uploaded_file_path(path));
	nftw_user_data_1 = discs;
	nftw_user_data_2 = num_discs;
	if (nftw(full_path, nftw_guess_track, 20, 0) == -1) {
		print_error_f(A_CYAN "\"%s\"" A_RESET " error occured while preparing tracks: " A_CYAN "%s\n", strerror(errno), full_path);
	}
	pop_string();
	for (int i = 0; i < *num_discs; i++) {
		qsort(discs[i].tracks, discs[i].num_tracks, sizeof(struct prepare_track_data), sort_prepare_track_num);
	}
}

static bool find_upload_path_by_prefix(char* dest, const char* prefix) {
	*dest = '\0';
	const char* uploads = get_property("path.uploads");
	DIR* dir = opendir(uploads);
	if (!dir) {
		print_error_f("Failed to read uploads directory: " A_CYAN "%s\n", uploads);
		return false;
	}
	struct dirent* entry = NULL;
	while (entry = readdir(dir)) {
		if (strstr(entry->d_name, prefix)) {
			strcpy(dest, entry->d_name);
			break;
		}
	}
	closedir(dir);
	return *dest != '\0';
}

void load_prepare(struct prepare_data* prepare, const char* prefix) {
	memset(prepare, 0, sizeof(struct prepare_data));
	char path[256];
	if (!find_upload_path_by_prefix(path, prefix)) {
		print_error_f("Did not find upload path based on prefix " A_CYAN "%s\n", prefix);
		return;
	}
	strcpy(prepare->filename, path + strlen(prefix) + 1);
	guess_album_name(prepare->album_name, prepare->filename);
	sprintf(prepare->released_at, "%i-01-01", guess_album_year(prepare->filename)); // TODO: full date
	strcpy(prepare->folder, path);
	strcpy(prepare->group_search.name, "groups");
	strcpy(prepare->group_search.type, "groups");

	guess_group_name(prepare->group_search.value, prepare->group_search.text, prepare->filename);

	// Remove the group name in the album name:
	char* group_name_in_album = strstr(prepare->album_name, prepare->group_search.text);
	if (group_name_in_album) {
		int group_name_length = strlen(prepare->group_search.text);
		replace_substring(prepare->album_name, group_name_in_album, prepare->album_name + group_name_length, sizeof(prepare->album_name), "");
		trim_ends(prepare->album_name, " \t");
	}

	guess_tracks(prepare->discs, &prepare->num_discs, path);
	// todo: sort tracks on the discs by their number.
	/*for (int i = 0; i < *num_discs; i++) {
		std::sort(discs[i].tracks, discs[i].tracks, [](const auto& left, const auto& right) {
			return left.num < right.num;
		});
	}*/

	guess_attachments(prepare->attachments, &prepare->num_attachments, path);
	guess_album_type(prepare->album_type, 10); //todo: num_tracks
	strcpy(prepare->album_release_type, "original"); // todo: guess
	guess_audio_format(prepare->audio_format, prepare->filename);
}

void load_add_group(struct add_group_data* add) {
	strcpy(add->country.name, "country");
	strcpy(add->country.type, "country");
	strcpy(add->country.value, "--");
	strcpy(add->country.text, "Unknown");
	strcpy(add->person.name, "id");
	strcpy(add->person.type, "person");
	strcpy(add->person.value, "0");
	strcpy(add->person.text, "");
}

static int sort_uploaded_albums(const void* a, const void* b) {
	long long prefix_a = 0;
	long long prefix_b = 0;
	sscanf(((struct upload_uploaded_data*)a)->prefix, "%lld", &prefix_a);
	sscanf(((struct upload_uploaded_data*)b)->prefix, "%lld", &prefix_b);
	long long diff = prefix_a - prefix_b;
	return diff > 0 ? -1 : (diff < 0 ? 1 : 0);
}

void load_upload(struct upload_data* upload) {
	int allocated = 100;
	upload->num_uploads = 0;
	upload->uploads = (struct upload_uploaded_data*)malloc(allocated * sizeof(struct upload_uploaded_data));
	if (!upload->uploads) {
		return;
	}
	memset(upload->uploads, 0, allocated * sizeof(struct upload_uploaded_data));
	const char* uploads = get_property("path.uploads");
	DIR* dir = opendir(uploads);
	if (!dir) {
		print_error_f("Failed to read uploads directory: " A_CYAN "%s\n", uploads);
		return;
	}
	struct dirent* entry = NULL;
	while (entry = readdir(dir)) {
		if (!is_dirent_directory(uploads, entry)) {
			continue;
		}
		char* space = strchr(entry->d_name, ' ');
		if (!space) {
			continue;
		}
		struct upload_uploaded_data* uploaded = &upload->uploads[upload->num_uploads];
		strncpy(uploaded->prefix, entry->d_name, space - entry->d_name);
		strcpy(uploaded->name, space + 1);
		upload->num_uploads++;
		if (upload->num_uploads >= allocated) {
			break;
		}
	}
	closedir(dir);
	qsort(upload->uploads, upload->num_uploads, sizeof(struct upload_uploaded_data), sort_uploaded_albums);
}
