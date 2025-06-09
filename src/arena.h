#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef struct Region {
	struct Region *next;
	size_t capacity;
	size_t size;
	char buffer[];
} Region;

Region *region_new(size_t capacity);

#define ARENA_DEFAULT_CAPACITY (640 * 1000)

typedef struct {
    Region *first;
    Region *last;
} Arena;

void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment);
void *arena_alloc(Arena *arena, size_t size);
void *arena_realloc(Arena *arena, void *old_ptr, size_t old_size, size_t new_size);
void arena_clean(Arena *arena);
void arena_free(Arena *arena);
void arena_summary(Arena *arena);

