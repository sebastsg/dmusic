#pragma once

#include "search.h"
#include "type.h"

#include <stdbool.h>

struct render_buffer;

struct import_attachment_data {
	char* name;
	char* link;
	char* preview;
	char* path;
	struct select_options targets;
	int selected_target;
};

struct import_track_data {
	int num;
	char fixed[128];
	char uploaded[256];
	char extension[16];
	char path[256];
};

struct import_disc_data {
	int num;
	char name[128];
	struct import_track_data tracks[128];
	int num_tracks;
};

struct import_data {
	char filename[512]; // without timestamp prefix
	char folder[512]; // with timestamp prefix
	char album_name[256]; // guessed album name
	char released_at[64];
	struct search_data group_search;
	struct import_attachment_data* attachments;
	struct import_disc_data* discs;
	int num_attachments;
	int allocated_attachments;
	int num_discs;
	int allocated_discs;
	char album_type[64];
	char album_release_type[64];
	char audio_format[32];
};

void load_import_attachment(struct import_attachment_data* attachment, const char* path);
void load_import(struct import_data* import, const char* prefix);
void render_import(struct render_buffer* buffer, const char* prefix);
void render_import_attachments(struct render_buffer* buffer, struct import_attachment_data* attachments, int count);
void render_import_attachment(struct render_buffer* buffer, const char* directory, const char* filename);
void render_import_disc(struct render_buffer* buffer, const char* key, struct import_disc_data* disc);
