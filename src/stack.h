#pragma once

void initialize_stack();
void free_stack();

char* top_string();
int size_of_top_string();
char* push_string(const char* string);
char* push_string_f(const char* format, ...);
char* append_top_string(const char* string);
char* append_top_string_f(const char* format, ...);
void pop_string();

char* copy_string(const char* string);
char* copy_string_f(const char* format, ...);

char* current_temporary_string();
char* replace_temporary_string(const char* format, ...);
