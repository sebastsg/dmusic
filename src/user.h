#pragma once

#include <stdbool.h>

struct render_buffer;

bool register_user(const char* name, const char* password);
bool login_user(const char* name, const char* password);

void render_profile(struct render_buffer* buffer);
void render_login(struct render_buffer* buffer);
void render_register(struct render_buffer* buffer);
