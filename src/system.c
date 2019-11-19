#include "system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

char* system_output(const char* command, size_t* out_size) {
	print_info_f(A_GREEN "%s", command);
	FILE* process = popen(command, "r");
	if (!process) {
		print_errno_f("Failed to run \"%s\"", command);
		return NULL;
	}
	size_t index = 0;
	size_t allocated_size = 512;
	char* buffer = (char*)malloc(allocated_size);
	if (!buffer) {
		print_error_f("Failed to allocate %zu bytes for system output buffer.", allocated_size);
		return NULL;
	}
	*buffer = '\0';
	size_t last_read_size = 0;
	while (last_read_size = fread(buffer + index, 1, allocated_size - index, process)) {
		index += last_read_size;
		if (index + 1 >= allocated_size) {
			char* new_buffer = (char*)realloc(buffer, allocated_size * 2);
			if (!new_buffer) {
				break;
			}
			buffer = new_buffer;
			allocated_size *= 2;
		}
	}
	pclose(process);
	if (out_size) {
		*out_size = index;
	}
	buffer[index] = '\0';
	return buffer;
}

void system_execute(const char* command) {
	print_info_f(A_GREEN "%s", command);
	system(command);
}
