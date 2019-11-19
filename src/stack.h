#pragma once

void initialize_stack();
void free_stack();

const char* push_string(const char* string);
const char* push_string_f(const char* format, ...);
void pop_string();

char* copy_string(const char* string);

const char* current_temporary_string();
const char* replace_temporary_string(const char* format, ...);
