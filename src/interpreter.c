#include "interpreter.h"
#include "build_command.h"
#include "da.h"
#include "expression.h"
#include "statement.h"
#include "symbol.h"
#include <signal.h>


Environment  environment_default() {
	Environment env = {
		// TODO: define some constants
		// COOK_VERSION=0.0.1
		// GCC_VERSION (get it from gcc itself) ...
		// DEBUG/RELEASE/MIN_SIZE_REL/DIST
	};
	return env;
}

Interpreter interpreter_new(Parser* p) {
	Interpreter in = {
		.parser = p,
	};
	in.root_env.current_build_command = (BuildCommand*)arena_alloc(&in.arena, sizeof(BuildCommand));
	*in.root_env.current_build_command = build_command_default();
	return in;
}


void interpreter_interpret(Interpreter* in) {
	BuildCommand* bc = interpreter_interpret_build_command(in);
	build_command_execute(bc);
}

void interpreter_dry_run(Interpreter* in) {
	BuildCommand* bc = interpreter_interpret_build_command(in);
	build_command_dump(bc, stdout);
}

BuildCommand* interpreter_interpret_build_command(Interpreter* in) {
	StatementList sl = parser_parse_all(in->parser);
	for (size_t i = 0; i < sl.count; ++i) {
		interpreter_execute(in, sl.items[i]);
	}
	return in->root_env.current_build_command;
}

void interpreter_error(Interpreter* in, Token token, const char* error_cstr) {
	in->had_error = true;
	fprintf(stderr,"[ERROR][interpreter] %zu:%zu %s\n\t%s %.*s\n",
		 token.line, token.column,
		 error_cstr, token_name_cstr(token), (int)token.str.count, token.str.items);
	raise(1);
}



SymbolValue interpreter_evaluate(Interpreter* in, Expression* e) {
	if (!e) { return (SymbolValue){0}; }

	switch (e->type) {
		case EXPR_VARIABLE: return interpreter_lookup_variable(in, e->variable.name.str, e);
		case EXPR_CALL: interpret_call(in, &e->call); break;
		case EXPR_CHAIN: {
			interpreter_evaluate(in, e->chain.right);
			return interpreter_evaluate(in, e->chain.expr);
		}
		case EXPR_LITERAL_STRING: {
			return (SymbolValue){
				.type = SYMBOL_VALUE_STRING,
				.string = e->literal_string.str,
			};
		}
		default: break;
	}
	return (SymbolValue){0};
}

void interpreter_execute(Interpreter* in, Statement* s) {
	switch (s->type) {
		case STATEMENT_BLOCK:      interpret_block(in, &s->block); return;
		case STATEMENT_EXPRESSION: interpreter_evaluate(in, s->expression.expression); return;
	}
}
void interpret_block(Interpreter* in, StatementBlock*  s) {
	for (size_t i = 0; i < s->statement_count; ++i) {
		interpreter_execute(in, s->statements[i]);
	}
}



void interpret_call(Interpreter* in, ExpressionCall* e) {
	SymbolValue callee = interpreter_evaluate(in, e->callee);

	if (callee.type == SYMBOL_VALUE_NIL) {
		interpreter_error(in, e->token, "callee is nil");
		return;
	}
	
	if (callee.type != SYMBOL_VALUE_METHOD) {
		interpreter_error(in, e->token, "callee is not a method");
		return;
	}

	// TODO: maybe arity check

	// append it to the current build context
	if (callee.method_type == METHOD_BUILD) {
		BuildCommand* bc = in->root_env.current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->target_names, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_COMPILER) {
		if (e->argc != 1) {
			interpreter_error(in, e->token, "compiler method takes only 1 argument");
			return;
		}
		BuildCommand* bc = in->root_env.current_build_command;
		SymbolValue arg = interpreter_evaluate(in, e->args[0]);
		bc->compiler = arg.string;
	} else if (callee.method_type == METHOD_INPUT) {
		BuildCommand* bc = in->root_env.current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->input_files, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_CFLAGS) {
		BuildCommand* bc = in->root_env.current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->cflags, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LDFLAGS) {
		BuildCommand* bc = in->root_env.current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->ldflags, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_SOURCE_DIR) {
		if (e->argc != 1) {
			interpreter_error(in, e->token, "source_dir method takes only 1 argument");
			return;
		}
		BuildCommand* bc = in->root_env.current_build_command;
		SymbolValue arg = interpreter_evaluate(in, e->args[0]);
		bc->source_dir = arg.string;
	} else if (callee.method_type == METHOD_OUTPUT_DIR) {
		if (e->argc != 1) {
			interpreter_error(in, e->token, "output_dir method takes only 1 argument");
			return;
		}
		BuildCommand* bc = in->root_env.current_build_command;
		SymbolValue arg = interpreter_evaluate(in, e->args[0]);
		bc->output_dir = arg.string;
	} else if (callee.method_type == METHOD_INCLUDE_DIR) {
		BuildCommand* bc = in->root_env.current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->include_dirs, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LIBRARY_DIR) {
		BuildCommand* bc = in->root_env.current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->library_dirs, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LINK) {
		BuildCommand* bc = in->root_env.current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->library_links, arg.string);
			}
		}
	}
}


SymbolValue interpreter_lookup_variable(Interpreter* in, StringView sv, Expression* e) {
	SymbolValue val = {0};
	val.string.items = sv.items;
	val.string.count = sv.count;

	#define METHOD(name, enm)                         \
		if (strncmp(name, sv.items, sv.count) == 0) { \
			val.type = SYMBOL_VALUE_METHOD;           \
			val.method_type = enm;                    \
			return val; }

	METHOD("build",       METHOD_BUILD);
	METHOD("compiler",    METHOD_COMPILER);
	METHOD("input",       METHOD_INPUT);
	METHOD("cflags",      METHOD_CFLAGS);
	METHOD("ldflags",     METHOD_LDFLAGS);
	METHOD("source_dir",  METHOD_SOURCE_DIR);
	METHOD("output_dir",  METHOD_OUTPUT_DIR);
	METHOD("include_dir", METHOD_INCLUDE_DIR);
	METHOD("library_dir", METHOD_LIBRARY_DIR);
	METHOD("link",        METHOD_LINK);

	return val;
}
