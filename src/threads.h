#pragma once

#include <stdbool.h>

void initialize_threads();
void update_threads();
void free_threads();

int open_thread(void(*function)(void*), void* data);
void join_thread_if_done(int id);
