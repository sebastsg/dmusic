#pragma once

#include <stddef.h>

enum resize_status {
	RESIZE_DONE,
	RESIZE_NOT_NEEDED,
	RESIZE_FAILED
};

enum resize_status resize_array(void** array, size_t element_size, int* allocated, int required);
