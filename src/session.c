#include "session.h"
#include "generic.h"
#include "system.h"
#include "files.h"
#include "config.h"
#include "database.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

static void delete_expired_sessions() {
	PQclear(execute_sql("delete from \"user_session\" where \"expires_at\" < current_timestamp", NULL, 0));
}

static void save_session(struct cached_session* session) {
	const char* params[] = { session->name, session->id };
	insert_row("user_session", false, 2, params);
}

static bool load_session(struct cached_session* session, const char* id) {
	const char* params[] = { id };
	PGresult* result = execute_sql("select \"user_name\" from \"user_session\" where \"id\" = $1 and \"expires_at\" > current_timestamp", params, 1);
	const bool exists = PQntuples(result) == 1;
	if (exists) {
		strcpy(session->id, id);
		strcpy(session->name, PQgetvalue(result, 0, 0));
	} else {
		print_error_f("Illegal request to load session %s, which does not exist.", id);
	}
	PQclear(result);
	return exists;
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
	delete_expired_sessions();
}

void update_session_state(struct cached_session* session) {
	memset(session->privileges, 0, sizeof(bool) * TOTAL_PRIVILEGES);
	memset(session->preferences, 0, sizeof(int) * TOTAL_PREFERENCES);
	const char* params[] = { session->name };
	PGresult* result = execute_sql("select \"privilege\" from \"user_privilege\" where \"user_name\" = $1", params, 1);
	const int num_privileges = PQntuples(result);
	for (int i = 0; i < num_privileges; i++) {
		const int privilege = atoi(PQgetvalue(result, i, 0));
		session->privileges[privilege] = true;
	}
	PQclear(result);
	result = execute_sql("select \"type\", \"preference\" from \"user_preference\" where \"user_name\" = $1", params, 1);
	const int num_preferences = PQntuples(result);
	for (int i = 0; i < num_preferences; i++) {
		const int type = atoi(PQgetvalue(result, i, 0));
		session->preferences[type] = atoi(PQgetvalue(result, i, 1));
	}
	PQclear(result);
}

const struct cached_session* create_session(const char* name) {
	resize_array((void**)&sessions, sizeof(struct cached_session), &allocated_sessions, loaded_sessions + 1);
	struct cached_session* session = NULL;
	for (int i = 0; i < loaded_sessions; i++) {
		if (strlen(sessions[i].id) == 0) {
			session = &sessions[i];
			break;
		}
	}
	if (!session) {
		session = &sessions[loaded_sessions];
	}
	generate_session_id(session->id);
	strcpy(session->name, name);
	update_session_state(session);
	save_session(session);
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

void delete_session(struct cached_session* session) {
	print_info_f("Deleting session " A_CYAN "%s.", session->id);
	const char* params[] = { session->id };
	PGresult* result = execute_sql("delete from \"user_session\" where \"id\" = $1", params, 1);
	PQclear(result);
	session->id[0] = '\0';
}

bool has_privilege(const struct cached_session* session, int privilege) {
	return session->privileges[privilege];
}

int get_preference(const struct cached_session* session, int preference) {
	return session->preferences[preference];
}

bool is_preferred(const struct cached_session* session, int preference, int value) {
	return get_preference(session, preference) == value;
}

void toggle_preference(struct cached_session* session, int preference, int if_this, int then_that) {
	session->preferences[preference] = is_preferred(session, preference, if_this) ? then_that : if_this;
}
