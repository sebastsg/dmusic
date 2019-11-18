#pragma once

struct render_buffer;

struct upload_uploaded_data {
	char prefix[32];
	char name[256];
};

struct upload_data {
	struct upload_uploaded_data* uploads;
	int num_uploads;
};

void load_upload(struct upload_data* upload);
void render_upload(struct render_buffer* buffer);
void render_remote_entries(struct render_buffer* buffer);
void render_sftp_ls(struct render_buffer* buffer);
void delete_upload(const char* prefix);
