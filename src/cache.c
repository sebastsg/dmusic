#include "cache.h"
#include "database.h"
#include "config.h"
#include "files.h"

#include <stdio.h>

static struct memory_cached_data memory_cache;

static void load_all_options() {
	load_options(&memory_cache.languages, "language");
	load_options(&memory_cache.countries, "country");
	load_options(&memory_cache.roles, "role");
	load_options(&memory_cache.album_types, "album_type");
	load_options(&memory_cache.album_release_types, "album_release_type");
	load_options(&memory_cache.audio_formats, "audio_format");
}

static void load_template(char** dest, const char* name) {
	static char path[256];
	*dest = read_file(html_path(path, sizeof(path), name), NULL);
	if (!*dest) {
		fprintf(stderr, "Failed to load template: %s\n", path);
	}
}

static void load_all_templates() {
	load_template(&memory_cache.main_template, "main");
	load_template(&memory_cache.session_track_template, "playlist_track");
	load_template(&memory_cache.select_option_template, "select_option");
	load_template(&memory_cache.group_thumb_template, "group_thumb");
	load_template(&memory_cache.group_thumb_list_template, "groups");
	load_template(&memory_cache.group_template, "group");
	load_template(&memory_cache.tag_link_template, "tag_link");
	load_template(&memory_cache.albums_template, "albums");
	load_template(&memory_cache.album_template, "album");
	load_template(&memory_cache.track_template, "track");
	load_template(&memory_cache.disc_template, "disc");
	load_template(&memory_cache.uploaded_albums_template, "uploaded_albums");
	load_template(&memory_cache.uploaded_album_template, "uploaded_album");
	load_template(&memory_cache.prepare_template, "prepare");
	load_template(&memory_cache.prepare_attachment_template, "prepare_attachment");
	load_template(&memory_cache.prepare_disc_template, "prepare_disc");
	load_template(&memory_cache.prepare_track_template, "prepare_track");
	load_template(&memory_cache.add_group_template, "add_group");
	load_template(&memory_cache.search_template, "search");
	load_template(&memory_cache.search_result_template, "search_result");
}

void load_memory_cache() {
	load_all_options();
	load_all_templates();
}

const struct memory_cached_data* mem_cache() {
	return &memory_cache;
}
