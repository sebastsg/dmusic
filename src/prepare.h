#pragma once

#include <stdbool.h>

struct select_options;

const char* guess_target(const char* filename);
int guess_targets(const char* filename, struct select_options* options);

int guess_album_year(const char* name);
char* guess_album_name(char* buffer, const char* name);

int guess_track_num(const char* name, bool less_than_100);
char* guess_track_name(char* buffer, const char* name);

int guess_disc_num(const char* track_name);
