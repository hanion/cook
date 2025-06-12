#pragma once
#include "arena.h"
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>

#define da_reserve(da, expected_capacity)                                                  \
	do {                                                                                   \
		if ((expected_capacity) > (da)->capacity) {                                        \
			if ((da)->capacity == 0) {                                                     \
				(da)->capacity = 256;                                                      \
			}                                                                              \
			while ((expected_capacity) > (da)->capacity) {                                 \
				(da)->capacity *= 2;                                                       \
			}                                                                              \
			(da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));     \
			assert((da)->items != NULL);                                                   \
		}                                                                                  \
	} while (0)

#define da_append(da, item)                    \
	do {                                       \
		da_reserve((da), (da)->count + 1);     \
		(da)->items[(da)->count++] = (item);   \
	} while (0)

#define da_append_many(da, new_items, new_items_count)                                          \
	do {                                                                                        \
		da_reserve((da), (da)->count + (new_items_count));                                      \
		memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items)); \
		(da)->count += (new_items_count);                                                       \
	} while (0)

typedef struct {
	char *items;
	size_t count;
	size_t capacity;
} StringBuilder;

static inline StringBuilder sb_new() {
	StringBuilder sb = {
		.items = NULL, .count = 0, .capacity = 0
	};
	return sb;
}
static inline void sb_free(StringBuilder* sb) {
	sb->count = 0;
	sb->capacity = 0;
	free(sb->items);
}

typedef struct {
	const char* items;
	size_t count;
} StringView;


typedef struct StringList {
	StringView* items;
	size_t count;
	size_t capacity;
} StringList;



static inline void string_list_add(Arena* arena, StringList* list, StringView sv) {
	if (list->count >= list->capacity) {
		size_t new_capacity = list->capacity ? list->capacity * 2 : 4;
		size_t new_size = new_capacity * sizeof(StringView);
		StringView* new_items = (StringView*)arena_alloc(arena, new_size);

		if (list->items) {
			memcpy(new_items, list->items, list->count * sizeof(StringView));
		}

		list->items = new_items;
		list->capacity = new_capacity;
	}

	list->items[list->count++] = sv;
}

