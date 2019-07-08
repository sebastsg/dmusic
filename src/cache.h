#pragma once

#include "data.h"

// todo: store cached files in array instead

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
	char* add_group_template;
	char* search_template;
	char* search_result_template;
	char* login_template;
	char* register_template;
	char* profile_template;

	char* style_css;
	char* main_js;
	char* icon_png;
	size_t icon_png_size;
	char* bg_jpg;
	size_t bg_jpg_size;
};

void load_memory_cache();
const struct memory_cached_data* mem_cache();
