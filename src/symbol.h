#pragma once
#include "arena.h"
#include "da.h"

typedef enum SymbolValueType {
	SYMBOL_VALUE_NIL = 0,
	SYMBOL_VALUE_INT,
	SYMBOL_VALUE_FLOAT,
	SYMBOL_VALUE_STRING,
	SYMBOL_VALUE_METHOD,
	SYMBOL_VALUE_BUILD_COMMAND,
} SymbolValueType;

typedef enum MethodType {
	METHOD_NONE = 0,
	METHOD_BUILD,
	METHOD_COMPILER,
	METHOD_INPUT,
	METHOD_CFLAGS,
	METHOD_LDFLAGS,
	METHOD_SOURCE_DIR,
	METHOD_OUTPUT_DIR,
	METHOD_INCLUDE_DIR,
	METHOD_LIBRARY_DIR,
	METHOD_LINK,
	METHOD_DIRTY,
	METHOD_ECHO,
} MethodType;

typedef struct SymbolValue {
	SymbolValueType type;
	union {
		int integer;
		float floating;
		StringView string;
		struct BuildCommand* bc;
		MethodType method_type;
	};
} SymbolValue;

typedef struct SymbolEntry {
	StringView name;
	SymbolValue value;
} SymbolEntry;

typedef struct SymbolMap {
	// TODO: use hash map
	// string -> symbol
	SymbolEntry** items;
	size_t count;
	size_t capacity;
} SymbolMap;


typedef struct Environment {
	struct Environment* enclosing;
	SymbolMap map;
} Environment;
Environment* environment_new(Arena*);


const char* symbol_value_type_name_cstr(SymbolValueType);
void symbol_value_print(SymbolValue value, int indent);

MethodType method_extract(StringView sv);
