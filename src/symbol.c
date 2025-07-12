#include "symbol.h"
#include "build_command.h"
#include <stdio.h>


// TODO: define some constants
// COOK_VERSION=0.0.1
// GCC_VERSION (get it from gcc itself) ...
// DEBUG/RELEASE/MIN_SIZE_REL/DIST
Environment* environment_new(Arena* arena) {
	Environment* env = (Environment*)arena_alloc(arena, sizeof(Environment));
	return env;
}

const char* symbol_value_type_name_cstr(SymbolValueType type) {
	switch (type) {
		#define CASE(T) case T: return #T;
		CASE(SYMBOL_VALUE_NIL)
		CASE(SYMBOL_VALUE_INT)
		CASE(SYMBOL_VALUE_FLOAT)
		CASE(SYMBOL_VALUE_STRING)
		CASE(SYMBOL_VALUE_METHOD)
		CASE(SYMBOL_VALUE_BUILD_COMMAND)
		#undef CASE
	}
	return "!!! SYMBOL VALUE TYPE INVALID";
}

void symbol_value_print(SymbolValue value, int indent) {
	for (int i = 0; i < indent; ++i) {
		putchar(' ');
	}
	printf("symbol ");
	switch (value.type) {
		case SYMBOL_VALUE_NIL:    printf("nil"); break;
		case SYMBOL_VALUE_INT:    printf("int: %d", value.integer); break;
		case SYMBOL_VALUE_FLOAT:  printf("float: %f", value.floating); break;
		case SYMBOL_VALUE_STRING: printf("string: %.*s", (int)value.string.count, value.string.items); break;
		case SYMBOL_VALUE_METHOD: printf("method: %.*s", (int)value.string.count, value.string.items); break;
		case SYMBOL_VALUE_BUILD_COMMAND:
			printf("build command:\n\t\t");
			build_command_print(value.bc, indent + 2);
			break;
		default: break;
	}
	printf("\n");
}

MethodType method_extract(StringView sv) {
	if (strncmp("build",       sv.items, sv.count) == 0) return METHOD_BUILD;
	if (strncmp("compiler",    sv.items, sv.count) == 0) return METHOD_COMPILER;
	if (strncmp("input",       sv.items, sv.count) == 0) return METHOD_INPUT;
	if (strncmp("cflags",      sv.items, sv.count) == 0) return METHOD_CFLAGS;
	if (strncmp("ldflags",     sv.items, sv.count) == 0) return METHOD_LDFLAGS;
	if (strncmp("source_dir",  sv.items, sv.count) == 0) return METHOD_SOURCE_DIR;
	if (strncmp("output_dir",  sv.items, sv.count) == 0) return METHOD_OUTPUT_DIR;
	if (strncmp("include_dir", sv.items, sv.count) == 0) return METHOD_INCLUDE_DIR;
	if (strncmp("library_dir", sv.items, sv.count) == 0) return METHOD_LIBRARY_DIR;
	if (strncmp("link",        sv.items, sv.count) == 0) return METHOD_LINK;
	if (strncmp("dirty",       sv.items, sv.count) == 0) return METHOD_DIRTY;
	if (strncmp("mark_clean",  sv.items, sv.count) == 0) return METHOD_MARK_CLEAN;
	if (strncmp("echo",        sv.items, sv.count) == 0) return METHOD_ECHO;

	return METHOD_NONE;
}
