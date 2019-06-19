#pragma once

#include "data.h"

struct memory_cached_data {
	struct select_options languages;
	struct select_options countries;
	struct select_options roles;
	struct select_options album_types;
	struct select_options album_release_types;
	struct select_options audio_formats;

	char* main_template;
	char* session_track_template;
	char* select_option_template;
	char* group_thumb_template;
	char* group_thumb_list_template;
	char* group_template;
	char* tag_link_template;
	char* albums_template;
	char* album_template;
	char* track_template;
	char* disc_template;
	char* uploaded_albums_template;
	char* uploaded_album_template;
	char* prepare_template;
	char* prepare_attachment_template;
	char* prepare_disc_template;
	char* prepare_track_template;
};

void load_memory_cache();
const struct memory_cached_data* mem_cache();
