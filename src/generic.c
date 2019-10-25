#include "generic.h"

#include <stdlib.h>

enum resize_status resize_array(void** array, size_t element_size, int* allocated, int required) {
	if (*allocated >= required) {
		return RESIZE_NOT_NEEDED;
	}
	required *= 2;
	const size_t new_size = element_size * required;
	if (*array) {
		void* new_array = realloc(*array, new_size);
		if (!new_array) {
			return RESIZE_FAILED;
		}
		*array = new_array;
	} else {
		*array = malloc(new_size);
		if (!*array) {
			return RESIZE_FAILED;
		}
	}
	*allocated = required;
	return RESIZE_DONE;
}
