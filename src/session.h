#pragma once

void initialize_sessions();
void free_sessions();

void open_session();
void close_session();

const char* get_session_username();
