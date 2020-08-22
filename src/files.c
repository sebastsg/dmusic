#include "files.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "stack.h"
#include "system.h"

static size_t min(size_t a, size_t b) {
	return a > b ? b : a;
}

FILE* open_file(const char* path, const char* mode) {
	if (!path) {
		print_error("Path is NULL.");
		return NULL;
	}
	FILE* file = fopen(path, mode);
	if (!file) {
		print_errno_f("Failed to open file %s.", path);
	}
	return file;
}

char* read_file(const char* path, size_t* size) {
	FILE* file = open_file(path, "rb");
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
		print_errno_f("I/O error while reading file %s.", path);
		return NULL;
	}
	fclose(file);
	if (size) {
		*size = file_size;
	}
	buffer[file_size] = '\0';
	return buffer;
}

char* read_file_section(const char* path, size_t* full_size, size_t* part_size, size_t begin, size_t end) {
	FILE* file = open_file(path, "rb");
	if (!file) {
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	if (file_size == 0) {
		return NULL;
	}
	if (begin >= file_size) {
		print_error_f("Can't start reading at %zu when filesize of %s is %zu.", begin, path, file_size);
		return NULL;
	}
	const size_t buffer_size = min(file_size - begin, end - begin);
	char* buffer = (char*)malloc(buffer_size + 1);
	if (!buffer) {
		return NULL;
	}
	fseek(file, begin, SEEK_SET);
	if (fread(buffer, 1, buffer_size, file) != buffer_size) {
		free(buffer);
		fclose(file);
		print_errno_f("I/O error while reading file %s.", path);
		return NULL;
	}
	fclose(file);
	if (full_size) {
		*full_size = file_size;
	}
	if (part_size) {
		*part_size = buffer_size;
	}
	buffer[buffer_size] = '\0';
	return buffer;
}

bool write_file(const char* path, const char* data, size_t size) {
	FILE* file = open_file(path, "wb");
	if (!file) {
		return false;
	}
	size_t written = fwrite(data, 1, size, file);
	fclose(file);
	if (written != size) {
		print_errno_f("Failed to write %zu bytes to %s. %zu bytes were written.", size, path, written);
	}
	return written == size;
}

bool file_exists(const char* path) {
	struct stat sb;
	if (stat(path, &sb) != -1) {
		return S_ISREG(sb.st_mode);
	} else if (errno != ENOENT) {
		print_errno_f("Failed to check file: " A_CYAN "%s", path);
	}
	return false;
}

bool directory_exists(const char* path) {
	struct stat sb;
	return stat(path, &sb) != -1 && S_ISDIR(sb.st_mode);
}

bool create_directory(const char* path) {
	bool success = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1;
	if (!success) {
		print_errno_f("Failed to create directory: " A_CYAN "%s", path);
	}
	return success;
}

bool is_dirent_directory(const char* root_path, struct dirent* entry) {
	if (entry->d_type == DT_DIR) {
		return true;
	} else if (entry->d_type == DT_UNKNOWN) {
		return directory_exists(replace_temporary_string("%s/%s", root_path, entry->d_name));
	} else {
		return false;
	}
}

bool is_dirent_file(const char* root_path, struct dirent* entry) {
	if (entry->d_type == DT_REG) {
		return true;
	} else if (entry->d_type == DT_UNKNOWN) {
		return file_exists(replace_temporary_string("%s/%s", root_path, entry->d_name));
	} else {
		return false;
	}
}
