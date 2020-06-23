#pragma once

#include <stddef.h>

struct render_buffer;

struct upload_uploaded_data {
	char prefix[32];
	char name[256];
};

struct upload_data {
	struct upload_uploaded_data* uploads;
	int num_uploads;
};

struct file_stats {
	size_t group_jpg;
	size_t group_png;
	size_t album_jpg;
	size_t album_png;
	size_t album_flac;
	size_t album_mp3;
	size_t uploads_total;
};

void load_stats_async();

void load_upload(struct upload_data* upload);
void render_upload(struct render_buffer* buffer);
void render_remote_entries(struct render_buffer* buffer);
void render_sftp_ls(struct render_buffer* buffer);
void delete_upload(const char* prefix);
