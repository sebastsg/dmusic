#include "files.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

char* read_file(const char* path, size_t* size) {
	FILE* file = fopen(path, "rb");
	if (!file) {
		fprintf(stderr, "Failed to open file %s for reading. Error: %s\n", path, strerror(errno));
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
		fprintf(stderr, "I/O error while reading file %s. Error: %s\n", path, strerror(errno));
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
		fprintf(stderr, "Failed to open file %s for writing. Error: %s\n", path, strerror(errno));
		return false;
	}
	size_t written = fwrite(data, 1, size, file);
	fclose(file);
	if (written != size) {
		fprintf(stderr, "Failed to write %zu bytes to %s. %zu bytes were written. Error: %s\n", size, path, written, strerror(errno));
	}
	return written == size;
}

bool file_exists(const char* path) {
	struct stat sb;
	return stat(path, &sb) != -1 && S_ISREG(sb.st_mode);
}

bool create_directory(const char* path) {
	bool success = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1;
	if (!success) {
		fprintf(stderr, "Failed to create directory. Error: %s\n", strerror(errno));
	}
	return success;
}
