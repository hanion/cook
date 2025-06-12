#pragma once
#include "da.h"
#include "symbol.h"
#include <stdio.h>


typedef enum BuildType {
	BUILD_EXECUTABLE,
	BUILD_OBJECT,
	BUILD_LIB,
} BuildType;

typedef struct BuildCommand BuildCommand;

struct BuildCommand {
	BuildCommand* parent;

	BuildCommand** children;
	size_t child_count;

	StringView compiler;

	BuildType build_type;

	// NOTE: we should not inherit root input
	StringList target_names;
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

BuildCommand build_command_default();
BuildCommand build_command_inherit(BuildCommand* parent);
void         build_command_print  (BuildCommand* bc);
void         build_command_execute(BuildCommand* bc);
void         build_command_dump   (BuildCommand* bc, FILE* stream);
