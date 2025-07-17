#include "build_command.h"
#include "da.h"

#define INDENT_MULTIPLIER 4

BuildCommand* build_command_new(Arena* arena) {
	BuildCommand* bc = (BuildCommand*)arena_alloc(arena, sizeof(BuildCommand));
	*bc = build_command_default();
	return bc;
}

BuildCommand build_command_default(void) {
	static const StringView gcc = { .items = "gcc", .count = 3 };

	BuildCommand bc = {0};
	bc.compiler = gcc;
	bc.build_type = BUILD_EXECUTABLE;
	return bc;
}

BuildCommand* build_command_inherit(Arena* arena, BuildCommand* parent) {
	if (!parent) {
		return NULL;
	}
	BuildCommand* bc = build_command_new(arena);

	bc->parent = parent;
	bc->compiler = parent->compiler;
	bc->include_dirs = parent->include_dirs;
	bc->library_dirs = parent->library_dirs;
	bc->cflags = parent->cflags;
	bc->ldflags = parent->ldflags;
	bc->source_dir = parent->source_dir;
	bc->output_dir = parent->output_dir;

	if (parent->parent != NULL) {
		bc->build_type = BUILD_OBJECT;
	}

	da_append_arena(arena, &parent->children, bc);
	return bc;
}


inline static void indent_label(int indent, const char* label) {
	printf("%*s%-14s: ", indent * INDENT_MULTIPLIER, "", label);
}
inline static void indent_label_sv(int indent, StringView sv) {
	printf("%*s%-14.*s: ", indent * INDENT_MULTIPLIER, "", (int)sv.count, sv.items);
}
inline static void string_list_print_big(int indent, const char* label, const StringList* list) {
	if (list->count == 0) return;

	indent_label(indent, label);
	for (size_t i = 0; i < list->count; ++i) {
		const StringView* sv = &list->items[i];
		printf("%.*s", (int)sv->count, sv->items);
		if (i + 1 < list->count) {
			printf(", ");
		}
	}
	printf("\n");
}

inline static void target_list_print_pretty(size_t indent, TargetList list) {
	if (list.count == 0) return;

	indent_label(indent, "targets");
	printf("\n");

	for (size_t i = 0; i <  list.count; ++i) {
		Target* t = &list.items[i];
		indent_label_sv(indent+1, t->name);
		printf("input: %-25.*s ", (int)t->input_name.count, t->input_name.items);
		printf("output: %-25.*s ", (int)t->output_name.count, t->output_name.items);
		if (t->dirty) printf("[dirty]");
		printf("\n");
	}
}

void build_command_print(BuildCommand* bc, size_t indent) {
	if (!bc) {
		return;
	}
	size_t ni = indent + 1;

	indent_label(ni, "build command");
	if (bc->dirty) printf("[dirty]");
	printf("\n");


	if (bc->compiler.count > 0) {
		indent_label(ni, "compiler");
		printf("%.*s\n", (int)bc->compiler.count, bc->compiler.items);
	}

	indent_label(ni, "build type");
	build_type_print(bc->build_type);
	printf("\n");

	target_list_print_pretty(ni, bc->targets);
	string_list_print_big(ni, "input files", &bc->input_files);
	string_list_print_big(ni, "input objects", &bc->input_objects);
	string_list_print_big(ni, "include dirs", &bc->include_dirs);
	string_list_print_big(ni, "include files", &bc->include_files);
	string_list_print_big(ni, "library dirs", &bc->library_dirs);
	string_list_print_big(ni, "library links", &bc->library_links);
	string_list_print_big(ni, "cflags", &bc->cflags);
	string_list_print_big(ni, "ldflags", &bc->ldflags);

	if (bc->source_dir.count > 0) {
		indent_label(ni, "source dir");
		printf("%.*s\n", (int)bc->source_dir.count, bc->source_dir.items);
	}
	if (bc->output_dir.count > 0) {
		indent_label(ni, "output dir");
		printf("%.*s\n", (int)bc->output_dir.count, bc->output_dir.items);
	}

	if (bc->children.count > 0) {
		indent_label(ni, "children");
		printf("%zu\n", bc->children.count);
		for (size_t i = 0; i < bc->children.count; ++i) {
			indent_label(indent + 2, "child");
			printf("%zu\n", i);
			build_command_print(bc->children.items[i], indent + 2);
		}
	}
}



void build_command_dump(Arena* arena, BuildCommand* bc, FILE* stream, size_t target_to_build) {
	if (!bc || !bc->dirty) {
		return;
	}

	// skip root bc
	if (!bc->parent) {
		for (size_t i = 0; i < bc->children.count; ++i) {
			build_command_dump(arena, bc->children.items[i], stream, 0);
		}
		return;
	}

	// recurse into children
	for (size_t i = 0; i < bc->children.count; ++i) {
		build_command_dump(arena, bc->children.items[i], stream, 0);
	}

	if (bc->targets.count == 0) {
		return;
	}
	if (bc->targets.count <= target_to_build) {
		return;
	}

	StringBuilder sb = target_generate_cmdline(arena, bc, &bc->targets.items[target_to_build]);

	printf("%.*s\n", (int)sb.count, sb.items);

	if (target_to_build == 0) {
		for (size_t i = 1; i < bc->targets.count; ++i) {
			build_command_dump(arena, bc, stream, i);
		}
	}
}

void build_type_print(BuildType type) {
	switch (type) {
		case BUILD_EXECUTABLE: printf("executable"); return;
		case BUILD_OBJECT:     printf("object");     return;
		case BUILD_LIB:        printf("lib");        return;
	}
}

void build_command_mark_all_targets_dirty(BuildCommand* bc, bool dirty) {
	if (!bc || (bc->marked_clean_explicitly==dirty)) return;
	for (size_t i = 0; i < bc->targets.count; ++i) {
		bc->targets.items[i].dirty = dirty;
	}
}
void build_command_mark_all_children_dirty(BuildCommand* bc, bool dirty) {
	if (!bc || (bc->marked_clean_explicitly==dirty)) return;
	bc->dirty = dirty;
	build_command_mark_all_targets_dirty(bc, dirty);
	for (size_t i = 0; i < bc->children.count; ++i) {
		build_command_mark_all_children_dirty(bc->children.items[i], dirty);
	}
}
