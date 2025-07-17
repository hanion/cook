#pragma once
#include "arena.h"
#include "da.h"
#include <stdbool.h>

typedef struct Target {
	StringView name;
	StringBuilder input_name;
	StringBuilder output_name;
	bool dirty;
} Target;

typedef struct TargetList {
	Target* items;
	size_t count;
	size_t capacity;
} TargetList;


struct BuildCommand;
StringBuilder target_generate_cmdline_cstr(Arena* arena, struct BuildCommand* bc, Target* t);
StringBuilder target_generate_cmdline     (Arena* arena, struct BuildCommand* bc, Target* t);

bool target_check_dirty(struct BuildCommand* bc, Target* t);

