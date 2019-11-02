#include "format.h"
#include "config.h"
#include "files.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char* image_extensions[] = { "jpg", "png", "jpeg", "gif", "webp", "bmp" };
static const char* audio_extensions[] = { "flac", "mp3", "m4a", "wav", "ogg" };

int count_in_string(const char* src, char symbol) {
	int i = 0;
	while (*src) {
		i += (*(src++) == symbol ? 1 : 0);
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

const char* split_string(char* dest, size_t size, const char* src, char symbol) {
	if (!src || !*src) {
		*dest = '\0';
		return NULL;
	}
	const char* i = src;
	while (*i && *(i++) != symbol);
	size_t offset = i - src;
	if (*i) {
		offset--;
	}
	offset = (offset >= size ? size - 1 : offset);
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
	return extension && any_string_contains(extension, image_extensions, NUM_ARRAY_ELEMENTS(image_extensions, char*));
}

bool is_extension_audio(const char* extension) {
	return extension && any_string_contains(extension, audio_extensions, NUM_ARRAY_ELEMENTS(audio_extensions, char*));
}

const char* find_file_extension(const char* path) {
	char buffer[256];
	const int extensions = NUM_ARRAY_ELEMENTS(image_extensions, char*);
	for (int i = 0; i < extensions; i++) {
		snprintf(buffer, sizeof(buffer), "%s%s", path, image_extensions[i]);
		if (file_exists(buffer)) {
			return image_extensions[i];
		}
	}
	return "";
}

char* string_copy_substring(char* dest, const char* src, size_t count) {
	strncpy(dest, src, count);
	dest[count] = '\0';
	return dest;
}

char* url_decode(char* url) {
	char* begin = url;
	char* end = url + strlen(url);
	char* dest = url;
	while (end > url) {
		if (*url != '%') {
			*(dest++) = *(url++);
			continue;
		}
		if (url + 3 > end) {
			break;
		}
		++url;
		bool hex_1 = (*url >= 'A' && *url <= 'F');
		bool dec_1 = (*url >= '0' && *url <= '9');
		char value_1 = *url - (dec_1 ? '0' : 'A' - 10);
		++url;
		bool hex_2 = (*url >= 'A' && *url <= 'F');
		bool dec_2 = (*url >= '0' && *url <= '9');
		char value_2 = *url - (dec_2 ? '0' : 'A' - 10);
		++url;
		if ((hex_1 || dec_1) && (hex_2 || dec_2)) {
			*(dest++) = 16 * value_1 + value_2;
		}
	}
	*dest = '\0';
	return begin;
}
