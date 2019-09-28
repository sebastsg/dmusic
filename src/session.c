#include "session.h"

#include <stdlib.h>
#include <string.h>

struct session_data {
	char id[16];
	char* username;
};

struct cached_session_list {
	struct session_data* sessions;
	int num_sessions;
};

static struct cached_session_list sessions;

void initialize_sessions() {
	sessions.num_sessions = 32;
	sessions.sessions = (struct session_data*)malloc(sessions.num_sessions * sizeof(struct session_data));
}

void free_sessions() {
	free(sessions.sessions);
	memset(&sessions, 0, sizeof(sessions));
}

void open_session() {
	
}

void close_session() {

}

const char* get_session_username() {
	return "dib";
}
