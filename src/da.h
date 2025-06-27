#pragma once
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

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


#define da_reserve_arena(arena, da, expected_capacity)                                     \
	do {                                                                                   \
		size_t old_size = (da)->capacity;                                                  \
		if ((expected_capacity) > (da)->capacity) {                                        \
			if ((da)->capacity == 0) {                                                     \
				(da)->capacity = 256;                                                      \
			}                                                                              \
			while ((expected_capacity) > (da)->capacity) {                                 \
				(da)->capacity *= 2;                                                       \
			}                                                                              \
			(da)->items = arena_realloc((arena), (da)->items,                              \
				old_size, (da)->capacity * sizeof(*(da)->items));                          \
			assert((da)->items != NULL);                                                   \
		}                                                                                  \
	} while (0)

#define da_append_arena(arena, da, item)                  \
	do {                                                  \
		da_reserve_arena((arena), (da), (da)->count + 1); \
		(da)->items[(da)->count++] = (item);              \
	} while (0)

#define da_append_many_arena(arena, da, new_items, new_items_count)                             \
	do {                                                                                        \
		da_reserve_arena((arena), (da), (da)->count + (new_items_count));                       \
		memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items)); \
		(da)->count += (new_items_count);                                                       \
	} while (0)


typedef struct {
	char *items;
	size_t count;
	size_t capacity;
} StringBuilder;

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




static inline StringView sv_from_sb(StringBuilder sb) {
	return (StringView) {
		.count = sb.count,
		.items = sb.items
	};
}
static inline StringBuilder sb_from_sv(StringView sv) {
	return (StringBuilder) {
		.count = sv.count,
		.items = (char*)sv.items,
		.capacity = sv.count
	};
}
