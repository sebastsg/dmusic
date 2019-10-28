#pragma once

#include "user.h"

#define SESSION_ID_SIZE 15

struct cached_session {
	char id[SESSION_ID_SIZE + 1];
	char name[32];
	bool privileges[TOTAL_PRIVILEGES];
	int preferences[TOTAL_PREFERENCES];
};

void initialize_sessions();
void free_sessions();

const struct cached_session* create_session(const char* name);
struct cached_session* open_session(const char* id);
void close_session(const char* id);

bool has_privilege(const struct cached_session* session, int privilege);
int get_preference(const struct cached_session* session, int preference);
bool is_preferred(const struct cached_session* session, int preference, int value);
void toggle_preference(struct cached_session* session, int preference, int if_this, int then_that);
