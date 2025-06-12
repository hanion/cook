#include "symbol.h"
#include "build_command.h"
#include <stdio.h>

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

inline static void print_indent(int indent) {
	for (int i = 0; i < indent; ++i) {
		putchar(' ');
	}
}
void symbol_value_print(SymbolValue value, int indent) {
	print_indent(indent);
	printf("symbol ");
	switch (value.type) {
		case SYMBOL_VALUE_NIL:    printf("nil"); break;
		case SYMBOL_VALUE_INT:    printf("int: %d", value.integer); break;
		case SYMBOL_VALUE_FLOAT:  printf("float: %f", value.floating); break;
		case SYMBOL_VALUE_STRING: printf("string: %.*s", (int)value.string.count, value.string.items); break;
		case SYMBOL_VALUE_METHOD: printf("method: %.*s", (int)value.string.count, value.string.items); break;
		case SYMBOL_VALUE_BUILD_COMMAND: printf("build command:\n\t\t"); build_command_print(value.bc); break;
		default: break;
	}
	printf("\n");
}
