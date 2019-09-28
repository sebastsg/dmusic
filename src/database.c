#include "database.h"
#include "config.h"
#include "files.h"
#include "format.h"
#include "stack.h"
#include "system.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static PGconn* session = NULL;

void connect_database() {
	if (session) {
		print_error("Already connected to database.");
		return;
	}
	char options[512];
	snprintf(options, sizeof(options), "host=%s port=%s dbname=%s user=%s password=%s", get_property("db.host"),
		get_property("db.port"), get_property("db.name"), get_property("db.user"), get_property("db.pass"));
	session = PQconnectdb(options);
	if (PQstatus(session) != CONNECTION_OK) {
		print_error_f("%s", PQerrorMessage(session));
		exit(1);
	}
}

void disconnect_database() {
	PQfinish(session);
	session = NULL;
}

PGresult* execute_sql(const char* query, const char** params, int count) {
	Oid* types = NULL; // deduced by backend
	int* lengths = NULL; // null-terminated, no need
	int* formats = NULL; // sending text, no need
	int result_format = 0; // text result
	PGresult* result = PQexecParams(session, query, count, types, params, lengths, formats, result_format);
	ExecStatusType error = PQresultStatus(result);
	if (error == PGRES_BAD_RESPONSE || error == PGRES_FATAL_ERROR || error == PGRES_NONFATAL_ERROR) {
		char* error_message = PQresultVerboseErrorMessage(result, PQERRORS_VERBOSE, PQSHOW_CONTEXT_ALWAYS);
		print_error_f("%s", error_message);
		PQfreemem(error_message);
	}
	return result;
}

PGresult* call_procedure(const char* procedure, const char** params, int count) {
	char query[256];
	int length = sprintf(query, "%s( ", procedure); // the space is important, see below
	for (int i = 1; i <= count; i++) {
		length += sprintf(query + length, "$%i,", i);
	}
	query[length - 1] = ')';
	return execute_sql(query, params, count);
}

void execute_sql_file(const char* name, bool split) {
	char* sql = read_file(server_sql_path(name), NULL);
	if (!sql) {
		print_error_f("Unable to read file: %s", name);
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

void find_best_audio_format(char* dest, int album_release_id, bool lossless) {
	*dest = '\0';
	char str[32];
	sprintf(str, "%i", album_release_id);
	const char* params[] = { str };
	PGresult* result = call_procedure("select * from get_album_formats", params, 1);
	int best_quality = -1;
	for (int i = 0; i < PQntuples(result); i++) {
		if (!lossless && atoi(PQgetvalue(result, i, 2)) == 1) {
			continue;
		}
		const char* code = PQgetvalue(result, i, 0);
		int quality = atoi(PQgetvalue(result, i, 1));
		if (quality > best_quality) {
			strcpy(dest, code);
			best_quality = quality;
		}
	}
	PQclear(result);
}

int first_album_attachment_of_type(int album_release_id, const char* type) {
	char id_str[32];
	sprintf(id_str, "%i", album_release_id);
	const char* params[] = { id_str, type };
	PGresult* result = execute_sql("select \"num\" from \"album_attachment\" where \"album_release_id\" = $1 and \"type\" = $2", params, 2);
	if (!result) {
		return 0;
	}
	int num = 0;
	if (PQntuples(result) > 0) {
		num = atoi(PQgetvalue(result, 0, 0));
	} else {
		print_error_f("No attachment of type " A_CYAN "%s" A_RED " was found for album " A_CYAN "%i", type, album_release_id);
	}
	PQclear(result);
	return num;
}

int first_group_attachment_of_type(int group_id, const char* type) {
	char id_str[32];
	sprintf(id_str, "%i", group_id);
	const char* params[] = { id_str, type };
	PGresult* result = execute_sql("select \"num\" from \"group_attachment\" where \"group_id\" = $1 and \"type\" = $2", params, 2);
	if (!result) {
		return 0;
	}
	int num = 0;
	if (PQntuples(result) > 0) {
		num = atoi(PQgetvalue(result, 0, 0));
	} else {
		print_error_f("No attachment of type " A_CYAN "%s" A_RED " was found for group " A_CYAN "%i", type, group_id);
	}
	PQclear(result);
	return num;
}
