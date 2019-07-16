#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#define NUM_ARRAY_ELEMENTS(ARRAY, TYPE) (int)(sizeof(ARRAY) / sizeof(TYPE))

/*
 * Replaces the elements @begin to @end with @src.
 * @dest:   Buffer where @dest[@size] is the last element.
 * @begin:  Where to start overwriting. Must be >= @dest.
 * @end:    Where to stop overwriting. Must be > @begin and <= @dest.
 * @size:   The allocated size of @dest. Must be >= (@end - @begin).
 * @src:    The replacement string.
 * Returns @dest.
 */
char* replace_substring(char* dest, char* begin, const char* end, size_t size, const char* src);

/*
 * Trims the left and right side of @str until not matching any of @symbols.
 * @str:     The string to trim.
 * @symbols: Characters that should be trimmed.
 * Returns trimmed @str.
 */
char* trim_ends(char* str, const char* symbols);

/*
 * Split the string at the symbol.
 * @dest:      Buffer for storing the next split string.
 * @size:      Allocated size for @dest.
 * @src:       Search string.
 * @symbol:    The split delimiter.
 * @discarded: If not null, the size that did not fit into @dest will be written to it.
 * Returns the next @source if there is more to read, or null if end of @source is reached.
 */
const char* split_string(char* dest, size_t size, const char* src, char symbol, size_t* discarded);

int count_in_string(const char* src, char symbol);
char* format_time(char* dest, size_t size, const char* format, time_t time);
char* make_duration_string(char* dest, int minutes, int seconds);
void erase_multi_space(char* str);
bool any_string_contains(const char* str, const char** needles, int count);
bool any_string_equal(const char* str, const char** needles, int count);
void replace_victims_with(char* str, const char* victims, char with);
bool is_extension_image(const char* extension);
bool is_extension_audio(const char* extension);
const char* find_file_extension(const char* path);

/*
 * Calls strncpy(@dest, @src, @count), then sets @dest[@count] to \0.
 * @dest:  Destination string.
 * @src:   Source string.
 * @count: How many characters to copy.
 * Returns @dest.
 */
char* string_copy_substring(char* dest, const char* src, size_t count);

/*
 * Decodes percent encoded strings in place.
 * @url: The string to decode. Will be modified.
 * Returns decoded @url.
 */
char* url_decode(char* url);
