#include "threads.h"
#include "system.h"
#include "generic.h"
#include "database.h"
#include "stack.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

enum thread_slot_state {
	THREAD_SLOT_AVAILABLE,
	THREAD_SLOT_RUNNING,
	THREAD_SLOT_READY_TO_JOIN
};

struct thread_slot {
	pthread_t thread;
	void (*function)(void*);
	void* data;
	int id;
	enum thread_slot_state status;
};

struct thread_slots {
	struct thread_slot** slots;
	int allocated;
	int count;
};

static struct thread_slots threads;

void initialize_threads() {
	memset(&threads, 0, sizeof(threads));
}

void update_threads() {
	for (int i = 0; i < threads.count; i++) {
		join_thread_if_done(i);
	}
}

void free_threads() {
	for (int i = 0; i < threads.count; i++) {
		join_thread_if_done(i);
		free(threads.slots[i]);
	}
	free(threads.slots);
}

struct thread_slot* find_available_thread_slot() {
	for (int i = 0; i < threads.count; i++) {
		if (threads.slots[i]->status == THREAD_SLOT_AVAILABLE) {
			return threads.slots[i];
		}
	}
	return NULL;
}

struct thread_slot* find_thread_slot(int id) {
	return id >= 0 && id < threads.count ? threads.slots[id] : NULL;
}

struct thread_slot* allocate_thread_slot() {
	resize_array((void**)&threads.slots, sizeof(struct thread_slot*), &threads.allocated, threads.count + 1);
	struct thread_slot* slot = (struct thread_slot*)calloc(1, sizeof(struct thread_slot));
	if (slot) {
		slot->id = threads.count;
		slot->status = THREAD_SLOT_AVAILABLE;
	}
	threads.slots[threads.count] = slot;
	threads.count++;
	return slot;
}

void* thread_wrapper(void* data) {
	struct thread_slot* slot = (struct thread_slot*)data;
	initialize_stack();
	connect_database();
	slot->function(slot->data);
	disconnect_database();
	free_stack();
	slot->status = THREAD_SLOT_READY_TO_JOIN;
	return NULL;
}

int open_thread(void(*function)(void*), void* data) {
	struct thread_slot* slot = find_available_thread_slot();
	if (!slot) {
		slot = allocate_thread_slot();
	}
	if (slot) {
		slot->function = function;
		slot->data = data;
		slot->status = THREAD_SLOT_RUNNING;
		if (pthread_create(&slot->thread, NULL, thread_wrapper, slot) != 0) {
			print_errno("Failed to create thread.");
			slot->status = THREAD_SLOT_AVAILABLE;
		}
	}
	return slot ? slot->id : -1;
}

void join_thread_if_done(int id) {
	struct thread_slot* slot = find_thread_slot(id);
	if (slot && slot->status == THREAD_SLOT_READY_TO_JOIN) {
		if (pthread_join(slot->thread, NULL) == 0) {
			slot->status = THREAD_SLOT_AVAILABLE;
		} else {
			print_errno_f("Failed to join thread %i.", slot->id);
		}
	}
}
