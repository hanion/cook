#include "interpreter.h"
#include "arena.h"
#include "build_command.h"
#include "da.h"
#include "expression.h"
#include "statement.h"
#include "symbol.h"
#include <signal.h>
#include <stdio.h>

static const SymbolValue nil = { .type = SYMBOL_VALUE_NIL };


// TODO: define some constants
// COOK_VERSION=0.0.1
// GCC_VERSION (get it from gcc itself) ...
// DEBUG/RELEASE/MIN_SIZE_REL/DIST
Environment* environment_new(Arena* arena) {
	Environment* env = (Environment*)arena_alloc(arena, sizeof(Environment));
	env->current_build_command = (BuildCommand*)arena_alloc(arena, sizeof(BuildCommand));
	*env->current_build_command = build_command_default();
	return env;
}

Interpreter interpreter_new(Parser* p) {
	Interpreter in = {
		.parser = p,
		.verbose = false,
	};
	in.current_environment = environment_new(&in.arena);
	return in;
}


void interpreter_interpret(Interpreter* in) {
	BuildCommand* bc = interpreter_interpret_build_command(in);
	build_command_execute(bc);
}

void interpreter_dry_run(Interpreter* in) {
	BuildCommand* bc = interpreter_interpret_build_command(in);
	if (in->verbose) {
		printf("[interpreter] build command pretty:\n");
		build_command_print(bc, 1);
		printf("[interpreter] build command dump:\n");
		build_command_dump(bc, stdout);
	}
}

BuildCommand* interpreter_interpret_build_command(Interpreter* in) {
	StatementList sl = parser_parse_all(in->parser);
	for (size_t i = 0; i < sl.count; ++i) {
		interpreter_execute(in, sl.items[i]);
	}
	return in->current_environment->current_build_command;
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
		case EXPR_CALL:     return interpret_call(in, &e->call);
		case EXPR_CHAIN:    return interpret_chain(in, &e->chain);
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

SymbolValue interpreter_execute(Interpreter* in, Statement* s) {
	switch (s->type) {
		case STATEMENT_BLOCK:       return interpret_block(in, &s->block);
		case STATEMENT_DESCRIPTION: return interpret_description(in, &s->description);
		case STATEMENT_EXPRESSION:  return interpreter_evaluate(in, s->expression.expression);
	}
	return nil;
}

SymbolValue interpret_block(Interpreter* in, StatementBlock*  s) {
	assert(false);
	return nil;
}

SymbolValue interpret_description(Interpreter* in, StatementDescription* s) {
	BuildCommand* enclosing = in->current_environment->current_build_command;

	SymbolValue left = interpreter_execute(in, s->statement);

	if (left.type == SYMBOL_VALUE_BUILD_COMMAND) {
		in->current_environment->current_build_command = left.bc;
	}

	for (size_t i = 0; i < s->block->block.statement_count; ++i) {
		interpreter_execute(in, s->block->block.statements[i]);
	}

	in->current_environment->current_build_command = enclosing;
	return left;
}

SymbolValue interpret_chain(Interpreter* in, ExpressionChain* e) {
	SymbolValue left = interpreter_evaluate(in, e->left);
	BuildCommand* enclosing = in->current_environment->current_build_command;
	if (left.type == SYMBOL_VALUE_BUILD_COMMAND) {
		in->current_environment->current_build_command = left.bc;
	}
	interpreter_evaluate(in, e->right);
	in->current_environment->current_build_command = enclosing;
	return left;
}

// NOTE: how do we know when to exit the current build command ?
// desc     -> {   build()  .input() ... }
// chain    -> {   build()  .input() ... }
//             {   build()  .input() ... }
// no chain -> { { build() } input() ... }

SymbolValue interpret_call(Interpreter* in, ExpressionCall* e) {
	SymbolValue callee = interpreter_evaluate(in, e->callee);

	if (callee.type == SYMBOL_VALUE_NIL) {
		interpreter_error(in, e->token, "callee is nil");
		return nil;
	}
	
	if (callee.type != SYMBOL_VALUE_METHOD) {
		interpreter_error(in, e->token, "callee is not a method");
		return nil;
	}

	// TODO: maybe arity check

	// append it to the current build context
	if (callee.method_type == METHOD_BUILD) {
		BuildCommand* enclosing = in->current_environment->current_build_command;
		BuildCommand* bc = build_command_inherit(&in->arena, in->current_environment->current_build_command);

		in->current_environment->current_build_command = bc;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &in->current_environment->current_build_command->target_names, arg.string);
			}
		}
		in->current_environment->current_build_command = enclosing;

		return (SymbolValue){
			.type = SYMBOL_VALUE_BUILD_COMMAND,
			.bc = bc
		};
	} else if (callee.method_type == METHOD_INPUT) {
		BuildCommand* bc = in->current_environment->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->input_files, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_COMPILER) {
		if (e->argc != 1) {
			interpreter_error(in, e->token, "compiler method takes only 1 argument");
			return nil;
		}
		BuildCommand* bc = in->current_environment->current_build_command;
		SymbolValue arg = interpreter_evaluate(in, e->args[0]);
		bc->compiler = arg.string;
	} else if (callee.method_type == METHOD_CFLAGS) {
		BuildCommand* bc = in->current_environment->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->cflags, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LDFLAGS) {
		BuildCommand* bc = in->current_environment->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->ldflags, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_SOURCE_DIR) {
		if (e->argc != 1) {
			interpreter_error(in, e->token, "source_dir method takes only 1 argument");
			return nil;
		}
		BuildCommand* bc = in->current_environment->current_build_command;
		SymbolValue arg = interpreter_evaluate(in, e->args[0]);
		bc->source_dir = arg.string;
	} else if (callee.method_type == METHOD_OUTPUT_DIR) {
		if (e->argc != 1) {
			interpreter_error(in, e->token, "output_dir method takes only 1 argument");
			return nil;
		}
		BuildCommand* bc = in->current_environment->current_build_command;
		SymbolValue arg = interpreter_evaluate(in, e->args[0]);
		bc->output_dir = arg.string;
	} else if (callee.method_type == METHOD_INCLUDE_DIR) {
		BuildCommand* bc = in->current_environment->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->include_dirs, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LIBRARY_DIR) {
		BuildCommand* bc = in->current_environment->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->library_dirs, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LINK) {
		BuildCommand* bc = in->current_environment->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->library_links, arg.string);
			}
		}
	}
	return nil;
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
