#pragma once

#include <stdio.h>

#define A_RESET    "\x1B[0m"
#define A_RED      "\x1B[31m"
#define A_GREEN    "\x1B[32m"
#define A_YELLOW   "\x1B[33m"
#define A_BLUE     "\x1B[34m"
#define A_MAGENTA  "\x1B[35m"
#define A_CYAN     "\x1B[36m"
#define A_WHITE    "\x1B[37m"

#define print_info_f(STR, ...)  printf(A_YELLOW STR A_RESET "\n", __VA_ARGS__)
#define print_info(STR)         puts(A_YELLOW STR A_RESET "\n")

#define print_error_f(STR, ...) fprintf(stderr, A_RED STR A_RESET "\n", __VA_ARGS__)
#define print_error(STR)        fputs(A_RED STR A_RESET "\n", stderr)

char* system_output(const char* command, size_t* size);
