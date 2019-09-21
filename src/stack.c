#include "stack.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct memory_block {
	char* data;
	int size;
};

struct memory_block_list {
	struct memory_block* blocks;
	int allocated;
	int next;
};

static bool is_memory_block_list_full(const struct memory_block_list* stack) {
	return stack->next >= stack->allocated;
}

static void resize_string_stack(struct memory_block_list* stack, int margin) {
	if (stack->blocks) {
		if (stack->allocated - stack->next > margin) {
			return;
		}
		const int new_allocated = stack->allocated + margin;
		struct memory_block* blocks = (struct memory_block*)realloc(stack->blocks, sizeof(struct memory_block) * new_allocated);
		printf("Resized stack to %i blocks\n", new_allocated);
		if (blocks) {
			stack->blocks = blocks;
			memset(stack->blocks + stack->allocated, 0, sizeof(struct memory_block) * margin);
			stack->allocated = new_allocated;
		}
	} else {
		stack->blocks = (struct memory_block*)calloc(margin * 2, sizeof(struct memory_block));
		printf("Allocated stack to %i blocks\n", margin * 2);
		if (stack->blocks) {
			stack->allocated = margin * 2;
		}
	}
}

static struct memory_block_list block_stack;

void initialize_stack() {
	memset(&block_stack, 0, sizeof(struct memory_block_list));
}

void free_stack() {
	for (int i = 0; i < block_stack.allocated; i++) {
		free(block_stack.blocks[i].data);
		printf("Freed block %i\n", i);
	}
	free(block_stack.blocks);
	printf("Freed block stack.\n");
	initialize_stack();
}

const char* push_memory_block(const char* data, int size) {
	resize_string_stack(&block_stack, 1);
	if (is_memory_block_list_full(&block_stack)) {
		return "";
	}
	size++;
	struct memory_block* block = &block_stack.blocks[block_stack.next];
	if (block->data) {
		if (size > block_stack.blocks[block_stack.next].size) {
			char* new_data = (char*)realloc(block->data, size);
			printf("Resized block %i to %i bytes\n", block_stack.next, size);
			if (new_data) {
				block->data = new_data;
				block->size = size;
			} else {
				return "";
			}
		}
	} else {
		block->data = (char*)malloc(size);
		printf("Allocated block %i to %i bytes\n", block_stack.next, size);
		if (!block->data) {
			return "";
		}
		block_stack.blocks[block_stack.next].size = size;
	}
	memcpy(block->data, data, size);
	block->data[size - 1] = '\0';
	block_stack.next++;
	return block->data;
}

const char* push_string(const char* string) {
	return push_memory_block(string, (int)strlen(string));
}

void pop_string() {
	block_stack.next--;
}

char* copy_string(const char* string) {
	char* copy = (char*)malloc(strlen(string) + 1);
	if (copy) {
		strcpy(copy, string);
	}
	return copy;
}
