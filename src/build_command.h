#pragma once
#include "arena.h"
#include "da.h"
#include "symbol.h"
#include <stdio.h>


typedef struct Target {
	StringView name;
	StringBuilder input_name;
	StringBuilder output_name;
} Target;

typedef struct TargetList {
	Target* items;
	size_t count;
	size_t capacity;
} TargetList;


typedef enum BuildType {
	BUILD_EXECUTABLE,
	BUILD_OBJECT,
	BUILD_LIB,
} BuildType;

typedef struct BuildCommand BuildCommand;

typedef struct BuildCommandChildren {
	BuildCommand** items;
	size_t count;
	size_t capacity;
} BuildCommandChildren;

struct BuildCommand {
	BuildCommand* parent;

	BuildCommandChildren children;

	StringView compiler;

	BuildType build_type;

	size_t target_to_build;
	TargetList targets;

	StringList input_files;

	StringList include_dirs;
	StringList include_files;

	StringList library_dirs;
	StringList library_links;

	StringList cflags;
	StringList ldflags;

	StringView source_dir;
	StringView output_dir;
};

BuildCommand* build_command_new(Arena*);

BuildCommand build_command_default();
void         build_command_print  (BuildCommand* bc, size_t indent);
void         build_command_execute(BuildCommand* bc);
void         build_command_dump   (BuildCommand* bc, FILE* stream);

BuildCommand* build_command_inherit(Arena* arena, BuildCommand* parent);

void build_type_print(BuildType type);

