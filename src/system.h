#pragma once

#include <stdio.h>
#include <errno.h>

#define A_RESET    "\x1B[0m"
#define A_RED      "\x1B[31m"
#define A_GREEN    "\x1B[32m"
#define A_YELLOW   "\x1B[33m"
#define A_BLUE     "\x1B[34m"
#define A_MAGENTA  "\x1B[35m"
#define A_CYAN     "\x1B[36m"
#define A_WHITE    "\x1B[37m"

#define PREPROCESSOR_TO_STRING_(X) #X
#define PREPROCESSOR_TO_STRING(X)  PREPROCESSOR_TO_STRING_(X)

#ifndef LOG_CONFIG_PREPEND_SOURCE
#define LOG_CONFIG_PREPEND_SOURCE 1
#endif

#ifndef LOG_CONFIG_VERBOSE
#define LOG_CONFIG_VERBOSE 0
#endif

#if LOG_CONFIG_PREPEND_SOURCE
#define LOG_SOURCE_FORMAT A_WHITE __FILE__ ":%s:" PREPROCESSOR_TO_STRING(__LINE__) ": "
#define LOG_SOURCE_ARGS   __func__
#define LOG_HAS_ARGS
#else
#define LOG_SOURCE_FORMAT
#define LOG_SOURCE_ARGS
#endif

#define LOG_BEGIN LOG_SOURCE_FORMAT
#define LOG_END   A_RESET "\n"

#ifdef LOG_HAS_ARGS
#define LOG_ARGS                  LOG_SOURCE_ARGS
#define LOG_INFO_F_(FORMAT, ...)  printf(LOG_BEGIN A_YELLOW FORMAT LOG_END, __VA_ARGS__)
#define LOG_INFO_F(FORMAT, ...)   LOG_INFO_F_(FORMAT, LOG_ARGS, ##__VA_ARGS__)
#define LOG_INFO(FORMAT)          LOG_INFO_F(FORMAT)
#define LOG_ERROR_F_(FORMAT, ...) fprintf(stderr, LOG_BEGIN A_RED FORMAT LOG_END, __VA_ARGS__)
#define LOG_ERROR_F(FORMAT, ...)  LOG_ERROR_F_(FORMAT, LOG_ARGS, ##__VA_ARGS__)
#define LOG_ERROR(FORMAT)         LOG_ERROR_F(FORMAT)
#else
#define LOG_INFO_F(FORMAT, ...)   printf(LOG_BEGIN A_YELLOW FORMAT LOG_END, __VA_ARGS__)
#define LOG_INFO(FORMAT)          puts(LOG_BEGIN A_YELLOW FORMAT LOG_END)
#define LOG_ERROR_F(FORMAT, ...)  fprintf(stderr, LOG_BEGIN A_RED FORMAT LOG_END, __VA_ARGS__)
#define LOG_ERROR(FORMAT)         fputs(LOG_BEGIN A_RED FORMAT LOG_END, stderr)
#endif

#define print_info_f(STR, ...)    LOG_INFO_F(STR, __VA_ARGS__)
#define print_info(STR)           LOG_INFO(STR)

#define print_errno_f(STR, ...)   LOG_ERROR_F(STR A_RED " Error: " A_CYAN "%s", __VA_ARGS__, strerror(errno))
#define print_errno(STR, ...)     LOG_ERROR_F(STR A_RED " Error: " A_CYAN "%s", strerror(errno))
#define print_error_f(STR, ...)   LOG_ERROR_F(STR, __VA_ARGS__)
#define print_error(STR)          LOG_ERROR(STR)

#if LOG_CONFIG_VERBOSE
#define print_verbose_f(STR, ...) LOG_INFO_F(STR, __VA_ARGS__)
#define print_verbose(STR)        LOG_INFO(STR)
#else
#define print_verbose_f(STR, ...)
#define print_verbose(STR)
#endif

char* system_output(const char* command, size_t* size);
