#include "stack.h"
#include "generic.h"
#include "system.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct memory_block {
	char* data;
	int size;
};

struct memory_block_list {
	struct memory_block* blocks;
	int allocated;
	int next;
};

struct string_buffer {
	char* value;
	size_t size;
};

static _Thread_local struct memory_block_list block_stack;
static _Thread_local struct string_buffer temporary_string;

static bool is_memory_block_list_full(const struct memory_block_list* stack) {
	return stack->next >= stack->allocated;
}

static void resize_buffer_stack(struct memory_block_list* stack, int margin) {
	const int allocated = stack->allocated;
	if (resize_array((void**)&stack->blocks, sizeof(struct memory_block), &stack->allocated, stack->next + 1) == RESIZE_DONE) {
		const int margin = stack->allocated - allocated;
		memset(stack->blocks + allocated, 0, sizeof(struct memory_block) * margin);
		print_info_f("Resized to %i blocks. (Next is %i)", stack->allocated, stack->next);
	}
}

void initialize_stack() {
	memset(&block_stack, 0, sizeof(struct memory_block_list));
	memset(&temporary_string, 0, sizeof(struct string_buffer));
}

void free_stack() {
	for (int i = 0; i < block_stack.allocated; i++) {
		free(block_stack.blocks[i].data);
		print_info_f("Freed block %i", i);
	}
	free(block_stack.blocks);
	print_info("Freed block stack.");
	free(temporary_string.value);
}

char* push_memory_block(const char* data, int size, int copy_size) {
	resize_buffer_stack(&block_stack, 1);
	if (is_memory_block_list_full(&block_stack)) {
		return NULL;
	}
	size++;
	struct memory_block* block = &block_stack.blocks[block_stack.next];
	if (resize_array((void**)&block->data, 1, &block->size, size) == RESIZE_FAILED) {
		return NULL;
	}
	if (copy_size > 0) {
		if (copy_size > size) {
			copy_size = size;
		}
		memcpy(block->data, data, copy_size);
		block->data[copy_size] = '\0';
	}
	block_stack.next++;
	return block->data;
}

const char* push_string(const char* string) {
	const int size = (int)strlen(string);
	char* data = push_memory_block(string, size, size);
	return data ? data : "";
}

const char* push_string_f(const char* format, ...) {
	va_list args;
	va_start(args, format);
	const int size = vsnprintf(NULL, 0, format, args);
	va_end(args);
	char* buffer = push_memory_block("", size + 1, 0);
	if (!buffer) {
		return "";
	}
	va_start(args, format);
	vsnprintf(buffer, size + 1, format, args);
	va_end(args);
	return buffer;
}

void pop_string() {
	if (block_stack.next > 0) {
		block_stack.next--;
	} else {
		print_error("Cannot pop string. Stack is already empty.");
	}
}

char* copy_string(const char* string) {
	char* copy = (char*)malloc(strlen(string) + 1);
	if (copy) {
		strcpy(copy, string);
	}
	return copy;
}

const char* current_temporary_string() {
	return temporary_string.value;
}

const char* replace_temporary_string(const char* format, ...) {
	if (!temporary_string.value) {
		temporary_string.value = (char*)malloc(32);
		if (temporary_string.value) {
			temporary_string.size = 32;
		} else {
			return "";
		}
	}
	va_list args;
	va_start(args, format);
	int size = vsnprintf(temporary_string.value, temporary_string.size, format, args);
	va_end(args);
	if (size >= (int)temporary_string.size) {
		char* new_value = (char*)realloc(temporary_string.value, size * 2);
		if (new_value) {
			temporary_string.value = new_value;
			temporary_string.size = size * 2;
			va_start(args, format);
			vsnprintf(temporary_string.value, temporary_string.size, format, args);
			va_end(args);
		} else {
			free(temporary_string.value);
			temporary_string.value = NULL;
			temporary_string.size = 0;
		}
	}
	return temporary_string.value ? temporary_string.value : "";
}
