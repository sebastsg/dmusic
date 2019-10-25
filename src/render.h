#pragma once

#include <stdbool.h>

struct tag_data;
struct cached_session;

struct render_buffer {
	char* data;
	int size;
	int offset;
};

char* render_resource(const char* resource, const struct cached_session* session);

void resize_render_buffer(struct render_buffer* buffer, int required_size);
void init_render_buffer(struct render_buffer* buffer, int size);
void assign_buffer(struct render_buffer* buffer, const char* str);
void append_buffer(struct render_buffer* buffer, const char* str);
void set_parameter(struct render_buffer* buffer, const char* key, const char* value);
void set_parameter_int(struct render_buffer* buffer, const char* key, int value);

void render_tags(struct render_buffer* buffer, const char* key, struct tag_data* tags, int num_tags);
void render_main(struct render_buffer* buffer, const char* resource, const struct cached_session* session);
