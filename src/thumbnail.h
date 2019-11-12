#pragma once

struct render_buffer;
struct search_result;

struct thumbnail {
	int id;
	char name[128];
	char* image;
};

void render_all_group_thumbnails(struct render_buffer* buffer);
void render_recent_group_thumbnails(struct render_buffer* buffer);
void render_recent_album_thumbnails(struct render_buffer* buffer);
void render_group_thumbnails_from_search(struct render_buffer* buffer, const char* type, const char* query);
void render_favourite_group_thumbnails(struct render_buffer* buffer, const char* user_name);
