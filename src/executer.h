#pragma once
#include "build_command.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
	Target** items;
	size_t count;
	size_t capacity;
} BuiltList;

typedef struct {
	BuildCommandList executed;
	BuiltList built;
	Arena* arena;
} Executer;

Executer executer_new(Arena* arena);
void executer_dry_run(Executer* e, BuildCommand* root);
void executer_execute(Executer* e, BuildCommand* root);


uint64_t get_modification_time_sv(StringView path);
uint64_t get_modification_time(const char *path_cstr);


