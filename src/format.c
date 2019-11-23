#include "format.h"
#include "config.h"
#include "files.h"
#include "stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system.h"

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
	while (*begin && strchr(symbols, *(begin++)))
		;
	while (*end && strchr(symbols, *(end--)))
		;
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
	while (*i && *(i++) != symbol)
		;
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
	const int extensions = NUM_ARRAY_ELEMENTS(image_extensions, char*);
	for (int i = 0; i < extensions; i++) {
		if (file_exists(replace_temporary_string("%s%s", path, image_extensions[i]))) {
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

static bool read_hex_digit(unsigned char* destination, char value) {
	const bool lower = (value >= 'a' && value <= 'f');
	const bool upper = (value >= 'A' && value <= 'F');
	const bool digit = (value >= '0' && value <= '9');
	*destination = (unsigned char)(value - (digit ? '0' : (upper ? 'A' : 'a') - 10));
	return lower || upper || digit;
}

static unsigned char* read_hex_byte(unsigned char* destination, char** data) {
	unsigned char a, b;
	const bool first = read_hex_digit(&a, *(*data)++);
	const bool second = read_hex_digit(&b, *(*data)++);
	if (first && second) {
		*destination = 16 * a + b;
		return destination + 1;
	} else {
		return destination;
	}
}

static void ucs2_decode(unsigned int value, char** buffer) {
	if (value < 0x80) {
		*(*buffer)++ = value;
	} else if (value < 0x800) {
		*(*buffer)++ = (value >> 6) | 0xC0;
		*(*buffer)++ = (value & 0x3F) | 0x80;
	} else if (value < 0xFFFF) {
		*(*buffer)++ = (value >> 12) | 0xE0;
		*(*buffer)++ = ((value >> 6) & 0x3F) | 0x80;
		*(*buffer)++ = (value & 0x3F) | 0x80;
	} else if (value <= 0x1FFFFF) {
		*(*buffer)++ = 0xF0 | (value >> 18);
		*(*buffer)++ = 0x80 | ((value >> 12) & 0x3F);
		*(*buffer)++ = 0x80 | ((value >> 6) & 0x3F);
		*(*buffer)++ = 0x80 | (value & 0x3F);
	}
}

static unsigned short read_hex_short(char** data) {
	unsigned char a, b, c, d;
	const bool first = read_hex_digit(&a, *(*data)++);
	const bool second = read_hex_digit(&b, *(*data)++);
	const bool third = read_hex_digit(&c, *(*data)++);
	const bool fourth = read_hex_digit(&d, *(*data)++);
	if (first && second && third && fourth) {
		a <<= 4;
		c <<= 4;
		a &= 0b11110000;
		b &= 0b00001111;
		c &= 0b11110000;
		d &= 0b00001111;
		unsigned short value = 0;
		value = a | b;
		value <<= 8;
		value |= c | d;
		return value;
	}
	return 0;
}

char* json_decode_string(char* string) {
	char* begin = string;
	char* end = string + strlen(string);
	char* dest = string;
	while (end > string) {
		if (*string == '\\' && *(string + 1) == 'u') {
			if (string + 5 > end) {
				break;
			}
			string += 2;
			unsigned short value = read_hex_short(&string);
			ucs2_decode(value, &dest);
		} else {
			*(dest++) = *(string++);
		}
	}
	*dest = '\0';
	return begin;
}

char* url_decode(char* url) {
	char* begin = url;
	char* end = url + strlen(url);
	unsigned char* dest = (unsigned char*)url;
	while (end > url) {
		if (*url != '%') {
			*(dest++) = (unsigned char)(*(url++));
			continue;
		}
		if (url + 3 > end) {
			break;
		}
		++url;
		dest = read_hex_byte(dest, &url);
	}
	*dest = '\0';
	return begin;
}

void string_replace(char* buffer, size_t size, const char* search, const char* replacement) {
	const size_t search_size = strlen(search);
	const size_t replacement_size = strlen(replacement);
	const size_t increment_size = search_size > replacement_size ? search_size : replacement_size;
	char* it = buffer;
	while (it = strstr(it, search)) {
		const size_t offset = it - buffer;
		if (offset + increment_size >= size) {
			break;
		}
		memmove(it + replacement_size, it + search_size, size - offset - increment_size);
		for (size_t i = 0; i < replacement_size; i++) {
			it[i] = replacement[i];
		}
		it += increment_size;
	}
}
