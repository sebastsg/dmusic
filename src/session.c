#include "session.h"
#include "generic.h"
#include "system.h"
#include "files.h"
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

static bool load_session(struct cached_session* session, const char* id) {
	const char* path = server_session_path(id);
	if (!file_exists(path)) {
		print_error_f("Illegal request to load session %s, which does not exist.", id);
		return false;
	}
	size_t size = 0;
	char* data = read_file(path, &size);
	if (!data) {
		print_error_f("Failed to load session %s. Error: %s.", id, strerror(errno));
		return false;
	}
	strcpy(session->id, id);
	char* begin = data;
	char* end = strchr(data, '\n');
	if (!end) {
		print_error_f("Username was not found in session %s.", id);
		return false;
	}
	*end = '\0';
	strcpy(session->name, begin);
	begin = end + 1;
	return true;
}

static void generate_session_id(char* id) {
	for (int i = 0; i < SESSION_ID_SIZE; i++) {
		sprintf(id + i, rand() % 2 == 0 ? "%x" : "%X", (unsigned char)(rand() % 16));
	}
}

static struct cached_session* sessions;
static int loaded_sessions;
static int allocated_sessions;

static struct cached_session* find_session(const char* id) {
	for (int i = 0; i < loaded_sessions; i++) {
		if (!strcmp(sessions[i].id, id)) {
			return &sessions[i];
		}
	}
	print_info_f("Did not find session " A_CYAN "%s", id);
	return NULL;
}

static bool save_session(const char* id) {
	struct cached_session* session = find_session(id);
	if (!session) {
		return false;
	}
	const char* path = server_session_path(id);
	char data[128];
	strcpy(data, session->name);
	strcat(data, "\n");
	write_file(path, data, strlen(data));
	return true;
}

void initialize_sessions() {
	sessions = NULL;
	loaded_sessions = 0;
	allocated_sessions = 0;
	resize_array((void**)&sessions, sizeof(struct cached_session), &allocated_sessions, 8);
}

void free_sessions() {
	free(sessions);
	sessions = NULL;
	loaded_sessions = 0;
	allocated_sessions = 0;
}

const struct cached_session* create_session(const char* name) {
	resize_array((void**)&sessions, sizeof(struct cached_session), &allocated_sessions, loaded_sessions + 1);
	struct cached_session* session = &sessions[loaded_sessions];
	generate_session_id(session->id);
	strcpy(session->name, name);
	print_info_f("Created session %i: " A_CYAN "%s" A_YELLOW " for " A_CYAN "%s", loaded_sessions, session->id, name);
	loaded_sessions++;
	return session;
}

struct cached_session* open_session(const char* id) {
	if (strlen(id) != SESSION_ID_SIZE) {
		return NULL;
	}
	struct cached_session* session = find_session(id);
	if (session) {
		return session;
	}
	resize_array((void**)&sessions, sizeof(struct cached_session), &allocated_sessions, loaded_sessions + 1);
	session = &sessions[loaded_sessions];
	if (load_session(session, id)) {
		loaded_sessions++;
		return session;
	} else {
		print_error_f("Failed to load session %s.", id);
		return NULL;
	}
}

void close_session(const char* id) {
	save_session(id);
}
