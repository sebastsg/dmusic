#pragma once

#include <stdbool.h>

enum user_privilege {
	PRIVILEGE_ENABLED,
	PRIVILEGE_EDIT_TAGS,
	PRIVILEGE_ADD_GROUP,
	PRIVILEGE_EDIT_GROUP_TAGS,
	PRIVILEGE_EDIT_GROUP_DETAILS,
	PRIVILEGE_UPLOAD_ALBUM,
	PRIVILEGE_IMPORT_ALBUM,
	PRIVILEGE_EDIT_ALBUM_DETAILS,
	PRIVILEGE_EDIT_LYRICS,
	PRIVILEGE_EDIT_TRACK_TAGS,
	PRIVILEGE_EDIT_TRACK_DETAILS,
	TOTAL_PRIVILEGES
};

enum user_preference {
	PREFERENCE_EDIT_MODE,
	PREFERENCE_PLAY_METHOD,
	TOTAL_PREFERENCES
};

enum edit_mode {
	EDIT_MODE_OFF,
	EDIT_MODE_ON
};

enum play_method {
	PLAY_METHOD_STREAM,
	PLAY_METHOD_PRELOAD,
	PLAY_METHOD_PRELOAD_IN_ADVANCE
};

struct render_buffer;

bool register_user(const char* name, const char* password);
bool login_user(const char* name, const char* password);

void render_profile(struct render_buffer* buffer);
void render_login(struct render_buffer* buffer);
void render_register(struct render_buffer* buffer);
