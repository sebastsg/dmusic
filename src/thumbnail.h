#pragma once

struct render_buffer;

struct thumbnail {
	int id;
	char name[128];
	char* image;
};

void render_all_group_thumbnails(struct render_buffer* buffer);
void render_recent_group_thumbnails(struct render_buffer* buffer);
void render_recent_album_thumbnails(struct render_buffer* buffer);
