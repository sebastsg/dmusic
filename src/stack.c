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
	int null_index;
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

static struct memory_block* top_memory_block() {
	return &block_stack.blocks[block_stack.next - 1];
}

static void resize_buffer_stack(struct memory_block_list* stack, int margin) {
	const int allocated = stack->allocated;
	if (resize_array((void**)&stack->blocks, sizeof(struct memory_block), &stack->allocated, stack->next + 1) == RESIZE_DONE) {
		const int margin = stack->allocated - allocated;
		memset(stack->blocks + allocated, 0, sizeof(struct memory_block) * margin);
	}
}

void initialize_stack() {
	memset(&block_stack, 0, sizeof(struct memory_block_list));
	memset(&temporary_string, 0, sizeof(struct string_buffer));
}

void free_stack() {
	if (block_stack.next > 0) {
		print_error_f("Freeing memory blocks while %i blocks are still pushed on the stack.", block_stack.next);
	}
	for (int i = 0; i < block_stack.allocated; i++) {
		free(block_stack.blocks[i].data);
	}
	free(block_stack.blocks);
	free(temporary_string.value);
}

char* push_memory_block(const char* data, int size, int copy_size) {
	resize_buffer_stack(&block_stack, 1);
	if (is_memory_block_list_full(&block_stack)) {
		return NULL;
	}
	size++;
	block_stack.next++;
	struct memory_block* block = top_memory_block();
	if (resize_array((void**)&block->data, 1, &block->size, size) == RESIZE_FAILED) {
		return NULL;
	}
	block->null_index = -1; // caller must set this if needed
	if (copy_size > 0) {
		if (copy_size > size) {
			copy_size = size;
		}
		memcpy(block->data, data, copy_size);
		block->data[copy_size] = '\0';
	}
	return block->data;
}

char* top_string() {
	return top_memory_block()->data;
}

int size_of_top_string() {
	return top_memory_block()->null_index;
}

char* push_string(const char* string) {
	const int size = (int)strlen(string);
	char* data = push_memory_block(string, size, size);
	top_memory_block()->null_index = size;
	return data ? data : "";
}

char* push_string_f(const char* format, ...) {
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
	top_memory_block()->null_index = size;
	return buffer;
}

char* append_top_string(const char* string) {
	struct memory_block* block = top_memory_block();
	const int suffix_size = (int)strlen(string);
	if (resize_array((void**)&block->data, 1, &block->size, block->null_index + suffix_size + 1) == RESIZE_FAILED) {
		return "";
	}
	strcpy(block->data + block->null_index, string);
	block->null_index += suffix_size;
	return block->data;
}

char* append_top_string_f(const char* format, ...) {
	struct memory_block* block = top_memory_block();
	va_list args;
	va_start(args, format);
	const int suffix_size = vsnprintf(NULL, 0, format, args);
	va_end(args);
	if (resize_array((void**)&block->data, 1, &block->size, block->null_index + suffix_size + 1) == RESIZE_FAILED) {
		return "";
	}
	va_start(args, format);
	vsnprintf(block->data + block->null_index, suffix_size + 1, format, args);
	va_end(args);
	block->null_index += suffix_size;
	return block->data;
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

char* copy_string_f(const char* format, ...) {
	va_list args;
	va_start(args, format);
	const int size = vsnprintf(NULL, 0, format, args) + 1;
	va_end(args);
	char* copy = (char*)malloc(size);
	if (copy) {
		va_start(args, format);
		vsnprintf(copy, size, format, args);
		va_end(args);
	}
	return copy;
}

char* current_temporary_string() {
	return temporary_string.value;
}

char* replace_temporary_string(const char* format, ...) {
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
