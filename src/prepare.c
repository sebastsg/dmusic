#include "prepare.h"
#include "format.h"
#include "data.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

// todo: clean up this function
int guess_targets(const char* filename, struct select_options* targets) {
	if (is_extension_audio(filename)) {
		return -1;
	}
	targets->count = 0;
	targets->options = (struct select_option*)malloc(5 * sizeof(struct select_option));
	if (!targets->options) {
		return -1;
	}
	int selected = 0;
	if (is_extension_image(filename)) {
		targets->count = 4;
		strcpy(targets->options[0].code, "cover");
		strcpy(targets->options[1].code, "back");
		strcpy(targets->options[2].code, "cd");
		strcpy(targets->options[3].code, "booklet");
	} else if (strstr(filename, ".pdf")) {
		targets->count = 1;
		strcpy(targets->options[0].code, "booklet");
	}
	const char* guess = guess_target(filename);
	bool found_guess = false;
	for (int i = 0; i < targets->count; i++) {
		if (strcmp(targets->options[i].code, guess) == 0) {
			selected = i;
			found_guess = true;
			break;
		}
	}
	if (!found_guess) {
		selected = targets->count;
		strcpy(targets->options[targets->count].code, guess);
		targets->count++; // already allocated
	}
	for (int i = 0; i < targets->count; i++) {
		strcpy(targets->options[i].name, targets->options[i].code);
	}
	return selected;
}

const char* guess_target(const char* name) {
	if (is_extension_image(name)) {
		if (strstr(name, "cover") || strstr(name, "folder")) {
			return "cover";
		}
		if (strstr(name, "back")) {
			return "back";
		}
		if (strstr(name, "cd")) {
			return "cd";
		}
		return "booklet";
	}
	if (strstr(name, ".pdf")) {
		return "booklet";
	}
	if (strstr(name, ".cue")) {
		return "cue";
	}
	if (strstr(name, ".log")) {
		return "log";
	}
	return "other";
}

// todo: store the compiled regex for reuse? especially considering if this is called in a loop.
// perhaps better to split into 2-3 functions, like how pcre works in the first place.
int regex_search(const char* pattern, const char** str) {
	int error = 0;
	PCRE2_SIZE error_offset = 0;
	pcre2_code* regex = pcre2_compile((PCRE2_SPTR8)pattern, PCRE2_ZERO_TERMINATED, 0, &error, &error_offset, NULL);
	if (!regex) {
		PCRE2_UCHAR error_str[128];
		pcre2_get_error_message(error, error_str, sizeof(error_str));
		fprintf(stderr, "Error compiling regex: \"%s\" at %i\n", error_str, (int)error_offset);
		return 0;
	}
	pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(regex, NULL);
	int rc = pcre2_match(regex, (PCRE2_SPTR)(*str), strlen(*str), 0, 0, match_data, NULL);
	int length = 0;
	if (rc >= 0) {
		PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);
		*str += ovector[0];
		length = ovector[1] - ovector[0];
	}
	pcre2_match_data_free(match_data);
	pcre2_code_free(regex);
	return length;
}

int guess_album_year(const char* name) {
	regex_search("(?<!\\d)(19|20)\\d{2}(?!\\d)", &name);
	return name ? atoi(name) : 0;
}

char* guess_album_name(char* dest, const char* name) {
	strcpy(dest, name);
	const int max_dest_size = strlen(dest);

	// Remove everything in [] {} and ()
	static const char* patterns[] = {
		"\\[([^\\]]+)\\]",
		"\\{([^\\}]+)\\}",
		"\\(([^\\)]+)\\)"
	};
	for (int i = 0; i < NUM_ARRAY_ELEMENTS(patterns, const char*); i++) {
		const char* it = dest;
		int length = 0;
		while (length = regex_search(patterns[i], &it)) {
			replace_substring(dest, (char*)it, it + length, max_dest_size, "");
		}
	}

	// Remove noise words:
	static const char* noise_words[] = {
		"V0", "v0", "CD V0", "V2", "v2", "CD V2",
		"WEB 128", "CD 128", "128k", "128K", "128",
		"WEB 192", "CD 192", "192k", "192K", "192",
		"WEB 320", "CD 320", "320k", "320K", "320",
		"WEB FLAC", "CD FLAC", "FLAC",
		"16Bit", "16 bit", "24Bit", "24 bit",
		"MP3"
	};
	for (int i = 0; i < NUM_ARRAY_ELEMENTS(noise_words, const char*); i++) {
		char* it = NULL;
		while (it = strstr(dest, noise_words[i])) {
			strcpy(it, it + strlen(noise_words[i]));
		}
	}

	// Remove release year:
	int year = guess_album_year(dest);
	char year_str[32];
	sprintf(year_str, "%i", year);
	char* found_year = strstr(dest, year_str);
	if (found_year) {
		replace_substring(dest, found_year, found_year + strlen(year_str), max_dest_size, "");
	}

	// Clean up:
	replace_victims_with(dest, "-_", ' ');
	erase_multi_space(dest);
	trim_ends(dest, " \t");

	return dest;
}

char* guess_track_num_raw(char* dest, const char* name) {
	int length = regex_search("\\(?\\d+\\s*[-.]?\\)?", &name);
	strncpy(dest, name, length);
	dest[length] = '\0';
	return dest;
}

int guess_track_num(const char* name, bool less_than_100) {
	char result[256];
	guess_track_num_raw(result, name);
	int length = strlen(result);
	if (length > 0 && !isdigit(result[0])) {
		// remove everything before first digit:
		for (int i = 1; i < length; i++) {
			if (isdigit(result[i - 1])) {
				strcpy(result, result + i);
				break;
			}
		}
	}
	int num = atoi(result);
	if (less_than_100) {
		while (num > 100) {
			num -= 100;
		}
	}
	return num;
}

char* guess_track_name(char* buffer, const char* name) {
	strcpy(buffer, name);
	char num_raw[256];
	guess_track_num_raw(num_raw, buffer);
	char* position = strstr(buffer, num_raw);
	if (position) {
		replace_substring(buffer, position, position + strlen(num_raw), 256, "");
	}
	replace_victims_with(buffer, "-_", ' ');
	erase_multi_space(buffer);
	trim_ends(buffer, " \t");
	char* dot = strrchr(buffer, '.');
	if (dot) {
		*dot = '\0';
	}
	return buffer;
}

int guess_disc_num(const char* track_name) {
	int track_num = guess_track_num(track_name, false);
	if (track_num == 0) {
		return 1;
	}
	char track_num_string[32];
	sprintf(track_num_string, "%i", track_num);
	if (strlen(track_num_string) > 2) {
		track_num_string[1] = '\0';
		int num = atoi(track_num_string);
		if (num > 0) {
			return num;
		}
	}
	return 1;
}
