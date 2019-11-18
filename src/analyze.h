#pragma once

#include <stdbool.h>
#include <stddef.h>

struct select_options;

const char* guess_file_extension(const char* data, size_t size);

void guess_group_name(char* value, char* text, const char* filename);
void guess_album_type(char* type, int num_tracks);
void guess_audio_format(char* format, const char* filename);

const char* guess_target(const char* filename);
int guess_targets(const char* filename, struct select_options* options);

int guess_album_year(const char* name);
char* guess_album_name(char* buffer, const char* name);

int guess_track_num(const char* name, bool less_than_100);
char* guess_track_name(char* buffer, const char* name);

int guess_disc_num(const char* track_name);
