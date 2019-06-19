#include "files.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

char* read_file(const char* path, size_t* size) {
	FILE* file = fopen(path, "rb");
	if (!file) {
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	if (file_size == 0) {
		return NULL;
	}
	char* buffer = (char*)malloc(file_size + 1);
	if (!buffer) {
		return NULL;
	}
	fseek(file, 0, SEEK_SET);
	if (fread(buffer, 1, file_size, file) != file_size) {
		free(buffer);
		fclose(file);
		return NULL;
	}
	fclose(file);
	if (size) {
		*size = file_size;
	}
	buffer[file_size] = '\0';
	return buffer;
}

bool write_file(const char* path, const char* data, size_t size) {
	FILE* file = fopen(path, "wb");
	if (!file) {
		return false;
	}
	size_t written = fwrite(data, 1, size, file);
	fclose(file);
	return written == size;
}

bool file_exists(const char* path) {
	struct stat sb;
	return stat(path, &sb) != -1;
}
