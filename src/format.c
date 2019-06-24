#include "format.h"
#include "config.h"
#include "data.h"
#include "files.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char* image_extensions[] = { "jpg", "jpeg", "png", "gif", "webp", "bmp" };
static const char* audio_extensions[] = { "flac", "mp3", "m4a", "wav", "ogg" };

int count_in_string(const char* source, char symbol) {
	int i = 0;
	while (*source) {
		i += (*(source++) == symbol ? 1 : 0);
	}
	return i;
}

char* replace_substring(char* dest, char* begin, const char* end, size_t size, const char* src) {
	char* temp = (char*)malloc(size);
	if (temp) {
		strcpy(temp, end);
		strcpy(begin, src);
		strcpy(begin + strlen(src), temp);
		free(temp);
	}
	return dest;
}

char* trim_ends(char* str, const char* symbols) {
	if (!*str) {
		return str;
	}
	size_t size = strlen(str);
	char* begin = str;
	char* end = str + size - 1;
	while (*begin && strchr(symbols, *(begin++)));
	while (*end && strchr(symbols, *(end--)));
	replace_substring(str, end + 2, str + size, size + 1, "");
	replace_substring(str, str, begin - 1, size + 1, "");
	return str;
}

const char* split_string(char* dest, size_t size, const char* src, char symbol, size_t* discarded) {
	if (!src || !*src) {
		return NULL;
	}
	const char* i = src;
	while (*i && *(i++) != symbol);
	size_t offset = i - src;
	if (offset-- == 1) {
		*dest = '\0';
		return i;
	}
	if (discarded) {
		*discarded = (offset > size ? offset - size : 0);
	}
	offset = (offset > size ? size : offset);
	memcpy(dest, src, offset);
	dest[offset] = '\0';
	return i;
}

char* format_time(char* str, size_t size, const char* format, time_t time) {
	strftime(str, size, format, localtime(&time));
	return str;
}

char* make_duration_string(char* str, int minutes, int seconds) {
	sprintf(str, "%i:%02i", minutes, seconds);
	return str;
}

void erase_multi_space(char* str) {
	char* dest = str;
	while (*str) {
		while ((*str == ' ' || *str == '\t') && (*(str + 1) == ' ' || *(str + 1) == '\t')) {
			str++;
			*str = ' '; // no tabs please
		}
		*(dest++) = *(str++);
	}
	*dest = '\0';
}

bool any_string_contains(const char* str, const char** needles, int count) {
	for (int i = 0; i < count; i++) {
		if (strstr(str, needles[i])) {
			return true;
		}
	}
	return false;
}

bool any_string_equal(const char* str, const char** needles, int count) {
	for (int i = 0; i < count; i++) {
		if (!strcmp(str, needles[i])) {
			return true;
		}
	}
	return false;
}

void replace_victims_with(char* str, const char* victims, char with) {
	while (*str) {
		if (strchr(victims, *str)) {
			*str = with;
		}
		str++;
	}
}

bool is_extension_image(const char* extension) {
	return any_string_contains(extension, image_extensions, NUM_ARRAY_ELEMENTS(image_extensions, char*));
}

bool is_extension_audio(const char* extension) {
	return any_string_contains(extension, audio_extensions, NUM_ARRAY_ELEMENTS(audio_extensions, char*));
}

const char* find_file_extension(const char* path) {
	char buffer[256];
	const int extensions = NUM_ARRAY_ELEMENTS(image_extensions, char*);
	for (int i = 0; i < extensions; i++) {
		strcpy(buffer, path);
		strcat(buffer, image_extensions[i]);
		if (file_exists(buffer)) {
			return image_extensions[i];
		}
	}
	return "";
}
