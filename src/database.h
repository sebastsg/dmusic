#pragma once

#include <postgresql/libpq-fe.h>
#include <stdbool.h>

struct select_options;

void connect_database();
void disconnect_database();
PGresult* execute_sql(const char* query, const char** params, int count);
PGresult* call_procedure(const char* procedure, const char** params, int count);
void execute_sql_file(const char* name, bool split);
int insert_row(const char* table, bool has_serial_id, int argc, const char** argv);

void find_best_audio_format(char* dest, int album_release_id, bool lossless);
int first_album_attachment_of_type(int album_release_id, const char* type);
int first_group_attachment_of_type(int group_id, const char* type);
