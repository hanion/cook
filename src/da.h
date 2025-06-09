#pragma once
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

// typedef struct {
// 	char * const items;
// 	size_t count;
// } StringView;
