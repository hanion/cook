#include "build_command.h"
#include <stdio.h>
#include <string.h>


BuildCommand build_command_default() {
	static const StringView gcc = { .items = "gcc", .count = 3 };

	BuildCommand bc = {0};
	bc.compiler = gcc;
	bc.build_type = BUILD_EXECUTABLE;
	return bc;
}

BuildCommand build_command_inherit(BuildCommand* parent) {
	BuildCommand new = (BuildCommand){
		// TODO:
	};
	return new;
}


inline static void indent_label(int indent, const char* label) {
	printf("%*s%16s: ", indent, "", label);
}
inline static void string_list_print_big(const char* label, const StringList* list, int indent) {
	if (list->count == 0) return;

	indent_label(indent, label);
	for (size_t i = 0; i < list->count; ++i) {
		const StringView* sv = &list->items[i];
		printf("%.*s ", (int)sv->count, sv->items);
	}
	printf("\n");
}
void build_command_print(BuildCommand* bc) {
	if (!bc) {
		return;
	}

	printf("build command:\n");

	if (bc->compiler.count > 0) {
		indent_label(2, "compiler");
		printf("%.*s\n", (int)bc->compiler.count, bc->compiler.items);
	}

	string_list_print_big("target names", &bc->target_names, 2);
	string_list_print_big("input files", &bc->input_files, 2);
	string_list_print_big("include dirs", &bc->include_dirs, 2);
	string_list_print_big("include files", &bc->include_files, 2);
	string_list_print_big("library dirs", &bc->library_dirs, 2);
	string_list_print_big("library links", &bc->library_links, 2);
	string_list_print_big("cflags", &bc->cflags, 2);
	string_list_print_big("ldflags", &bc->ldflags, 2);

	if (bc->source_dir.count > 0) {
		indent_label(2, "source dir");
		printf("%.*s\n", (int)bc->source_dir.count, bc->source_dir.items);
	}
	if (bc->output_dir.count > 0) {
		indent_label(2, "output dir");
		printf("%.*s\n", (int)bc->output_dir.count, bc->output_dir.items);
	}

	if (bc->child_count > 0) {
		printf("  children:\n");
		for (size_t i = 0; i < bc->child_count; ++i) {
			printf("\t  child %zu:\n", i);
			build_command_print(bc->children[i]);
		}
	}
}


inline static void print_stringview(FILE* stream, StringView sv) {
	fprintf(stream, "%.*s ", (int)sv.count, sv.items);
}

inline static void string_list_print_flat(FILE* stream, const StringList* list, const char* prefix) {
	for (size_t i = 0; i < list->count; ++i) {
		if (prefix) { fprintf(stream, "%s", prefix); }
		print_stringview(stream, list->items[i]);
	}
}

void build_command_dump(BuildCommand* bc, FILE* stream) {
	if (!bc) {
		return;
	}

	if (bc->compiler.count > 0) {
		print_stringview(stream, bc->compiler);
	}

	string_list_print_flat(stream, &bc->cflags, "");

	if (bc->target_names.count > 0) {
		fprintf(stream, "-o ");
		print_stringview(stream, bc->target_names.items[0]);
	}

	string_list_print_flat(stream, &bc->include_dirs, "-I");
	string_list_print_flat(stream, &bc->input_files, "");
	string_list_print_flat(stream, &bc->library_dirs, "-L");
	string_list_print_flat(stream, &bc->library_links, "-l");
	string_list_print_flat(stream, &bc->ldflags, "");

	fprintf(stream, "\n");

	// recurse into children
	for (size_t i = 0; i < bc->child_count; ++i) {
		build_command_dump(bc->children[i], stream);
	}
}

void build_command_execute(BuildCommand* bc) {
	build_command_dump(bc, stdout);
}
