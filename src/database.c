#include "database.h"
#include "config.h"
#include "files.h"
#include "data.h"
#include "format.h"

#include <postgresql/libpq-fe.h>
#include <openssl/sha.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#define DMUSIC_HASH_STRING_SIZE (SHA256_DIGEST_LENGTH * 2 + 1)
#define DMUSIC_SALT_SIZE        32
#define DMUSIC_SALT_STRING_SIZE (DMUSIC_SALT_SIZE * 2 + 1)

static PGconn* session = NULL;

void connect_database() {
	if (session) {
		printf("Already connected to database.\n");
		return;
	}
	char options[512];
	sprintf(options, "host=%s port=%s dbname=%s user=%s password=%s", get_property("db.host"),
		get_property("db.port"), get_property("db.name"), get_property("db.user"), get_property("db.pass"));
	//printf("Connecting to database with options: \"%s\"\n", options);
	session = PQconnectdb(options);
	if (PQstatus(session) != CONNECTION_OK) {
		fprintf(stderr, "%s\n", PQerrorMessage(session));
		exit(1);
	}
}

void disconnect_database() {
	PQfinish(session);
	session = NULL;
}

static PGresult* execute_sql(const char* query, const char** params, int count) {
	Oid* types = NULL; // deduced by backend
	int* lengths = NULL; // null-terminated, no need
	int* formats = NULL; // sending text, no need
	int result_format = 0; // text result
	PGresult* result = PQexecParams(session, query, count, types, params, lengths, formats, result_format);
	ExecStatusType error = PQresultStatus(result);
	//printf("SQL query: %s\n", query);
	if (error == PGRES_BAD_RESPONSE || error == PGRES_FATAL_ERROR || error == PGRES_NONFATAL_ERROR) {
		char* error_str = PQresultVerboseErrorMessage(result, PQERRORS_VERBOSE, PQSHOW_CONTEXT_ALWAYS);
		fprintf(stderr, "%s\n", error_str);
		PQfreemem(error_str);
	}
	return result;
}

static PGresult* call_procedure(const char* procedure, const char** params, int count) {
	char query[256];
	int length = sprintf(query, "%s( ", procedure); // the space is important, see below
	for (int i = 1; i <= count; i++) {
		length += sprintf(query + length, "$%i,", i);
	}
	query[length - 1] = ')';
	return execute_sql(query, params, count);
}

void execute_sql_file(const char* name, bool split) {
	char path[256];
	char* sql = read_file(sql_path(path, 256, name), NULL);
	if (!sql) {
		fprintf(stderr, "Unable to read file: %s\n", name);
		return;
	}
	if (split) {
		char* start = sql;
		char* current = sql;
		while (*(++sql)) { // use strchr?
			if (*sql == ';') {
				*sql = '\0';
				PQclear(execute_sql(current, NULL, 0));
				current = sql + 1;
			}
		}
		sql = start;
	} else {
		PQclear(execute_sql(sql, NULL, 0));
	}
	free(sql);
}

int insert_row(const char* table, bool has_serial_id, int argc, const char** argv) {
	char query[2048];
	sprintf(query, "insert into \"%s\" values ( ", table);
	if (has_serial_id) {
		strcat(query, "default,");
	}
	for (int i = 1; i <= argc; i++) {
		char param[16];
		sprintf(param, "$%i,", i);
		strcat(query, param);
	}
	query[strlen(query) - 1] = ')';
	if (has_serial_id) {
		strcat(query, " returning id");
	}
	PGresult* result = execute_sql(query, argv, argc);
	int id = 0;
	if (PQntuples(result) > 0) {
		id = atoi(PQgetvalue(result, 0, 0));
	}
	PQclear(result);
	return id;
}

int create_session_track(const char* user, int album_release_id, int disc_num, int track_num) {
	char album[32];
	char disc[32];
	char track[32];
	sprintf(album, "%i", album_release_id);
	sprintf(disc, "%i", disc_num);
	sprintf(track, "%i", track_num);
	const char* params[] = { user, album, disc, track };
	PGresult* result = call_procedure("select * from create_session_track", params, 4);
	int num = (PQntuples(result) > 0) ? atoi(PQgetvalue(result, 0, 0)) : 0;
	PQclear(result);
	return num;
}

void load_options(struct select_options* options, const char* type) {
	char query[128];
	sprintf(query, "select * from %s order by \"name\"", type);
	PGresult* result = execute_sql(query, NULL, 0);
	options->count = PQntuples(result);
	options->options = (struct select_option*)malloc(options->count * sizeof(struct select_option));
	if (options->options) {
		for (int i = 0; i < options->count; i++) {
			struct select_option* option = &options->options[i];
			strcpy(option->code, PQgetvalue(result, i, 0));
			strcpy(option->name, PQgetvalue(result, i, 1));
		}
	}
	PQclear(result);
}

void load_search_results(struct search_result_data** search_results, int* count, const char* type, const char* query) {
	char procedure[32];
	sprintf(procedure, "select * from search_%s", type);
	char search[32];
	char limit[32];
	sprintf(search, "%%%s%%", query);
	strcpy(limit, "10");
	const char* params[] = { search, limit };
	PGresult* result = call_procedure(procedure, params, 2);
	*count = PQntuples(result);
	if (*count == 0) {
		PQclear(result);
		return;
	}
	*search_results = (struct search_result_data*)malloc(*count * sizeof(struct search_result_data));
	if (!*search_results) {
		PQclear(result);
		return;
	}
	for (int i = 0; i < *count; i++) {
		struct search_result_data* search_result = &(*search_results)[i];
		strcpy(search_result->value, PQgetvalue(result, i, 0));
		strcpy(search_result->text, PQgetvalue(result, i, 1));
	}
	PQclear(result);
}

void load_group_name(char* dest, int id) {
	char id_str[32];
	sprintf(id_str, "%i", id);
	const char* params[] = { id_str };
	PGresult* result = execute_sql("select name from \"group\" where \"id\" = $1", params, 1);
	if (PQntuples(result) > 0) {
		strcpy(dest, PQgetvalue(result, 0, 0));
	} else {
		dest[0] = '\0';
	}
	PQclear(result);
}

void load_group_tags(struct tag_data** tags, int* num_tags, int id) {
	char id_str[32];
	sprintf(id_str, "%i", id);
	const char* params[] = { id_str };
	PGresult* result = call_procedure("select * from get_group_tags", params, 1);
	*num_tags = PQntuples(result);
	*tags = (struct tag_data*)malloc(*num_tags * sizeof(struct tag_data));
	if (*tags) {
		for (int i = 0; i < *num_tags; i++) {
			strcpy((*tags)[i].name, PQgetvalue(result, i, 0));
		}
	}
	PQclear(result);
}

void load_group_tracks(struct track_data** tracks, int* num_tracks, int group_id) {
	char id_str[32];
	sprintf(id_str, "%i", group_id);
	const char* params[] = { id_str };
	PGresult* result = call_procedure("select * from get_group_tracks", params, 1);
	*num_tracks = PQntuples(result);
	*tracks = (struct track_data*)malloc(*num_tracks * sizeof(struct track_data));
	if (*tracks) {
		for (int i = 0; i < *num_tracks; i++) {
			struct track_data* track = &(*tracks)[i];
			track->album_release_id = atoi(PQgetvalue(result, i, 0));
			track->disc_num = atoi(PQgetvalue(result, i, 1));
			track->num = atoi(PQgetvalue(result, i, 2));
			strcpy(track->name, PQgetvalue(result, i, 4));
		}
	}
	PQclear(result);
}

void load_group_albums(struct album_data** albums, int* num_albums, int group_id) {
	char id_str[32];
	sprintf(id_str, "%i", group_id);
	const char* params[] = { id_str };
	PGresult* result = call_procedure("select * from get_group_albums", params, 1);
	*num_albums = PQntuples(result);
	*albums = (struct album_data*)malloc(*num_albums * sizeof(struct album_data));
	if (*albums) {
		for (int i = 0; i < *num_albums; i++) {
			struct album_data* album = &(*albums)[i];
			album->id = atoi(PQgetvalue(result, i, 0));
			album->album_release_id = atoi(PQgetvalue(result, i, 4));
			format_time(album->released_at, 16, "%Y", (time_t)atoi(PQgetvalue(result, i, 7)));
			strcpy(album->name, PQgetvalue(result, i, 1));
			strcpy(album->album_type_code, PQgetvalue(result, i, 2));
			int cover_num = (PQgetisnull(result, i, 3) ? 0 : atoi(PQgetvalue(result, i, 3)));
			if (cover_num != 0) {
				client_album_image_path(album->image, album->album_release_id, cover_num);
			} else {
				strcpy(album->image, "/img/missing.png");
			}
		}
	}
	PQclear(result);
}

void load_group(struct group_data* group, int id) {
	load_group_name(group->name, id);
	load_group_tags(&group->tags, &group->num_tags, id);
	load_group_albums(&group->albums, &group->num_albums, id);
	load_group_tracks(&group->tracks, &group->num_tracks, id);
}

void load_disc_tracks(struct track_data** tracks, int* count, int album_release_id, int disc_num) {
	char album_release_id_str[32];
	char disc_num_str[32];
	sprintf(album_release_id_str, "%i", album_release_id);
	sprintf(disc_num_str, "%i", disc_num);
	const char* params[] = { album_release_id_str, disc_num_str };
	PGresult* result = execute_sql("select \"num\", \"name\" from \"track\" where \"album_release_id\" = $1 and \"disc_num\" = $2", params, 2);
	*count = PQntuples(result);
	*tracks = (struct track_data*)malloc(*count * sizeof(struct track_data));
	if (*tracks) {
		for (int i = 0; i < *count; i++) {
			struct track_data* track = &(*tracks)[i];
			track->album_release_id = album_release_id;
			track->disc_num = disc_num;
			track->num = atoi(PQgetvalue(result, i, 0));
			strcpy(track->name, PQgetvalue(result, i, 1));
		}
	}
	PQclear(result);
}

void load_album_tracks(struct track_data** tracks, int* count, int album_release_id) {
	char album_release_id_str[32];
	sprintf(album_release_id_str, "%i", album_release_id);
	const char* params[] = { album_release_id_str };
	PGresult* result = execute_sql("select \"disc_num\", \"num\", \"name\" from \"track\" where \"album_release_id\" = $1", params, 1);
	*count = PQntuples(result);
	*tracks = (struct track_data*)malloc(*count * sizeof(struct track_data));
	if (*tracks) {
		for (int i = 0; i < *count; i++) {
			struct track_data* track = &(*tracks)[i];
			track->album_release_id = album_release_id;
			track->disc_num = atoi(PQgetvalue(result, i, 0));
			track->num = atoi(PQgetvalue(result, i, 1));
			strcpy(track->name, PQgetvalue(result, i, 2));
		}
	}
	PQclear(result);
}

void load_session_tracks(struct session_track_data** tracks, int* num_tracks, const char* user) {
	const char* params[] = { user };
	PGresult* result = call_procedure("select * from get_session_tracks", params, 1);
	*num_tracks = PQntuples(result);
	if (*num_tracks == 0) {
		PQclear(result);
		return;
	}
	*tracks = (struct session_track_data*)malloc(*num_tracks * sizeof(struct session_track_data));
	if (*tracks) {
		for (int i = 0; i < *num_tracks; i++) {
			struct session_track_data* track = &(*tracks)[i];
			track->album_release_id = atoi(PQgetvalue(result, i, 0));
			track->disc_num = atoi(PQgetvalue(result, i, 1));
			track->track_num = atoi(PQgetvalue(result, i, 2));
			strcpy(track->name, PQgetvalue(result, i, 3));
			int seconds = atoi(PQgetvalue(result, i, 4));
			int minutes = seconds / 60;
			seconds = seconds % 60;
			make_duration_string(track->duration, minutes, seconds);
		}
	}
	PQclear(result);
}

void load_playlist_list(struct playlist_list_data* list, const char* user) {
	const char* params[] = { user };
	PGresult* result = call_procedure("select * from get_playlists_by_user", params, 1);
	list->num_playlists = PQntuples(result);
	list->playlists = (struct playlist_item_data*)malloc(list->num_playlists * sizeof(struct playlist_item_data));
	if (list->playlists) {
		for (int i = 0; i < list->num_playlists; i++) {
			struct playlist_item_data* playlist = &list->playlists[i];
			playlist->id = atoi(PQgetvalue(result, i, 0));
			strcpy(playlist->name, PQgetvalue(result, i, 1));
		}
	}
	PQclear(result);
}

void load_group_thumbs(struct group_thumb_data** thumbs, int* num_thumbs) {
	PGresult* result = execute_sql("select \"id\", name from \"group\"", NULL, 0);
	*num_thumbs = PQntuples(result);
	*thumbs = (struct group_thumb_data*)malloc(*num_thumbs * sizeof(struct group_thumb_data));
	if (*thumbs) {
		for (int i = 0; i < *num_thumbs; i++) {
			struct group_thumb_data* thumb = &(*thumbs)[i];
			thumb->id = atoi(PQgetvalue(result, i, 0));
			strcpy(thumb->name, PQgetvalue(result, i, 1));
			client_group_image_path(thumb->image, thumb->id, 1);
		}
	}
	PQclear(result);
}

static void make_random_salt(char* salt) {
	*salt = '\0';
	for (int i = 0; i < DMUSIC_SALT_SIZE;) {
		unsigned char value = (unsigned char)(rand() % 256);
		i += sprintf(salt + i, "%02X", value);
	}
}

static void make_sha256_hash(const char* password, const char* salt, char* dest) {
	unsigned char binary_hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, password, strlen(password));
	if (salt) {
		SHA256_Update(&sha256, salt, strlen(salt));
	}
	SHA256_Final(binary_hash, &sha256);
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		dest += sprintf(dest, "%02X", binary_hash[i]);
	}
}

bool register_user(const char* name, const char* password) {
	if (strlen(name) < 1 || strlen(password) < 8) {
		return false;
	}
	const char* params[] = { name };
	PGresult* result = execute_sql("select \"name\" from \"user\" where \"name\" = $1", params, 1);
	bool exists = PQntuples(result) > 0;
	PQclear(result);
	if (exists) {
		return false;
	}
	char hash[DMUSIC_HASH_STRING_SIZE];
	char salt[DMUSIC_SALT_STRING_SIZE];
	make_random_salt(salt);
	make_sha256_hash(password, salt, hash);
	const char* args[] = { name, hash, salt };
	insert_row("user", false, 3, args);
	return true;
}

bool login_user(const char* name, const char* password) {
	if (strlen(name) < 1 || strlen(password) < 8) {
		return false;
	}
	const char* params[] = { name, "" };
	PGresult* result = execute_sql("select \"salt\" from \"user\" where \"name\" = $1", params, 1);
	char salt[DMUSIC_SALT_STRING_SIZE];
	if (PQntuples(result) == 0) {
		PQclear(result);
		return false;
	}
	strcpy(salt, PQgetvalue(result, 0, 0));
	PQclear(result);
	char hash[DMUSIC_HASH_STRING_SIZE];
	make_sha256_hash(password, salt, hash);
	params[1] = hash;
	result = execute_sql("select \"name\" from \"user\" where \"name\" = $1 and \"password_hash\" = $2", params, 2);
	bool exists = PQntuples(result) > 0;
	PQclear(result);
	return exists;
}
