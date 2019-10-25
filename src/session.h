#pragma once

#define SESSION_ID_SIZE 15

struct cached_session {
	char id[SESSION_ID_SIZE + 1];
	char name[32];
};

void initialize_sessions();
void free_sessions();

const struct cached_session* create_session(const char* name);
struct cached_session* open_session(const char* id);
void close_session(const char* id);
