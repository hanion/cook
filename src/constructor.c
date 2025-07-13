#include "constructor.h"
#include "da.h"
#include "executer.h"
#include "statement.h"
#include "symbol.h"
#include <signal.h>
#include <stdint.h>

static const SymbolValue nill = { .type = SYMBOL_VALUE_NIL };

Constructor constructor_new(Statement* root_statement) {
	Constructor con = {0};
	con.current_statement = root_statement;
	con.current_environment = environment_new(&con.arena);
	con.current_build_command = build_command_new(&con.arena);
	return con;
}

BuildCommand* constructor_construct_build_command(Constructor* con) {
	Statement* root = con->current_statement;
	con->current_build_command->body = root;

	constructor_execute(con, root);
	constructor_expand_build_command_targets(con, con->current_build_command);
	constructor_analyze(con, con->current_build_command);
	con->current_build_command->dirty = true; // root build command is always dirty
	return con->current_build_command;
}


void constructor_analyze(Constructor* con, BuildCommand* bc) {
	if (!con || !bc) return;

	bool dirty_child = false;
	for (size_t i = 0; i < bc->children.count; ++i) {
		constructor_analyze(con, bc->children.items[i]);
		if (bc->children.items[i]->dirty) {
			dirty_child = true;
		}
	}
	if (dirty_child) {
		bc->dirty = true;
		return;
	}

	uint64_t oldest_target_time = INT64_MAX;
	for (size_t i = 0; i < bc->targets.count; ++i) {
		uint64_t t = get_modification_time_sv(sv_from_sb(bc->targets.items[i].output_name));
		if (t != 0 && t < oldest_target_time) {
			oldest_target_time = t;
		}
	}

	uint64_t newest_input_time = 0;
	for (size_t i = 0; i < bc->input_files.count; ++i) {
		uint64_t t = get_modification_time_sv(bc->input_files.items[i]);
		if (t != 0 && t > newest_input_time) {
			newest_input_time = t;
		}
	}
	for (size_t i = 0; i < bc->targets.count; ++i) {
		uint64_t t = get_modification_time_sv(sv_from_sb(bc->targets.items[i].input_name));
		if (t != 0 && t > newest_input_time) {
			newest_input_time = t;
		}
	}

	if (oldest_target_time < newest_input_time) {
		bc->dirty = true;
	}
}

void constructor_error(Constructor* con, Token token, const char* error_cstr) {
	con->had_error = true;
	fprintf(stderr,"[ERROR][constructor] %zu:%zu %s\n\t%s %.*s\n",
		 token.line + 1, token.column,
		 error_cstr, token_name_cstr(token), (int)token.str.count, token.str.items);
	raise(1);
}



SymbolValue constructor_evaluate(Constructor* con, Expression* e) {
	if (!e) { return nill; }

	switch (e->type) {
		case EXPR_VARIABLE: return constructor_lookup_variable(con, e->variable.name.str, e);
		case EXPR_CALL:     return constructor_interpret_call(con, &e->call);
		case EXPR_CHAIN:    return constructor_interpret_chain(con, &e->chain);
		case EXPR_LITERAL_STRING: {
			return (SymbolValue){
				.type = SYMBOL_VALUE_STRING,
				.string = e->literal_string.str,
			};
		}
		default: break;
	}
	return nill;
}

SymbolValue constructor_execute(Constructor* con, Statement* s) {
	con->current_statement = s;
	switch (s->type) {
		case STATEMENT_BLOCK:       return constructor_interpret_block(con, &s->block);
		case STATEMENT_DESCRIPTION: return constructor_interpret_description(con, &s->description);
		case STATEMENT_EXPRESSION:  return constructor_evaluate(con, s->expression.expression);
	}
	return nill;
}

SymbolValue constructor_interpret_block(Constructor* con, StatementBlock*  s) {
	for (size_t i = 0; i < s->statement_count; ++i) {
		constructor_execute(con, s->statements[i]);
	}
	return nill;
}

SymbolValue constructor_interpret_description(Constructor* con, StatementDescription* s) {
	BuildCommand* enclosing = con->current_build_command;
	Statement* outer = con->current_statement;

	SymbolValue left = constructor_execute(con, s->statement);

	if (left.type == SYMBOL_VALUE_BUILD_COMMAND) {
		con->current_build_command = left.bc;
		con->current_build_command->body = outer;
	}

	for (size_t i = 0; i < s->block->block.statement_count; ++i) {
		constructor_execute(con, s->block->block.statements[i]);
	}

	con->current_build_command = enclosing;
	return left;
}

SymbolValue constructor_interpret_chain(Constructor* con, ExpressionChain* e) {
	Statement* outer = con->current_statement;
	SymbolValue left = constructor_evaluate(con, e->left);
	BuildCommand* enclosing = con->current_build_command;
	if (left.type == SYMBOL_VALUE_BUILD_COMMAND) {
		con->current_build_command = left.bc;
		con->current_build_command->body = outer;
	}
	constructor_evaluate(con, e->right);
	con->current_build_command = enclosing;
	return left;
}

SymbolValue constructor_interpret_call(Constructor* con, ExpressionCall* e) {
	SymbolValue callee = constructor_evaluate(con, e->callee);
	Token callee_token = {
		.type = TOKEN_IDENTIFIER,
		.str.count = callee.string.count,
		.str.items = callee.string.items,
		.line = e->token.line,
		.column = e->token.column,
	};

	if (callee.type == SYMBOL_VALUE_NIL) {
		constructor_error(con, callee_token, "callee is nil");
		return nill;
	}
	
	if (callee.type != SYMBOL_VALUE_METHOD) {
		constructor_error(con, callee_token, "callee is not a method");
		return nill;
	}

	if (callee.method_type == METHOD_BUILD) {
		return constructor_interpret_method_build(con, e);
	} else if (callee.method_type == METHOD_INPUT) {
		BuildCommand* bc = con->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = constructor_evaluate(con, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&con->arena, &bc->input_files, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_COMPILER) {
		if (e->argc != 1) {
			constructor_error(con, e->token, "compiler method takes only 1 argument");
			return nill;
		}
		BuildCommand* bc = con->current_build_command;
		SymbolValue arg = constructor_evaluate(con, e->args[0]);
		bc->compiler = arg.string;
	} else if (callee.method_type == METHOD_CFLAGS) {
		BuildCommand* bc = con->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = constructor_evaluate(con, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&con->arena, &bc->cflags, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LDFLAGS) {
		BuildCommand* bc = con->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = constructor_evaluate(con, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&con->arena, &bc->ldflags, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_SOURCE_DIR) {
		if (e->argc != 1) {
			constructor_error(con, e->token, "source_dir method takes only 1 argument");
			return nill;
		}
		BuildCommand* bc = con->current_build_command;
		SymbolValue arg = constructor_evaluate(con, e->args[0]);
		bc->source_dir = arg.string;
	} else if (callee.method_type == METHOD_OUTPUT_DIR) {
		if (e->argc != 1) {
			constructor_error(con, e->token, "output_dir method takes only 1 argument");
			return nill;
		}
		BuildCommand* bc = con->current_build_command;
		SymbolValue arg = constructor_evaluate(con, e->args[0]);
		bc->output_dir = arg.string;
	} else if (callee.method_type == METHOD_INCLUDE_DIR) {
		BuildCommand* bc = con->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = constructor_evaluate(con, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&con->arena, &bc->include_dirs, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LIBRARY_DIR) {
		BuildCommand* bc = con->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = constructor_evaluate(con, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&con->arena, &bc->library_dirs, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LINK) {
		BuildCommand* bc = con->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = constructor_evaluate(con, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&con->arena, &bc->library_links, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_DIRTY) {
		BuildCommand* bc = con->current_build_command;
		bc->dirty = true;
		while (bc->parent) {
			bc->parent->dirty = true;
			bc = bc->parent;
		}
	} else if (callee.method_type == METHOD_MARK_CLEAN) {
		con->current_build_command->marked_clean_explicitly = true;
	}
	return nill;
}


SymbolValue constructor_lookup_variable(Constructor* con, StringView sv, Expression* e) {
	SymbolValue val = {0};
	val.string.items = sv.items;
	val.string.count = sv.count;
	val.type = SYMBOL_VALUE_STRING;

	MethodType m = method_extract(sv);
	if (m != METHOD_NONE) {
		val.type = SYMBOL_VALUE_METHOD;
		val.method_type = m;
	}

	return val;
}

SymbolValue constructor_interpret_method_build(Constructor* con, ExpressionCall* e) {
	Statement* outer = con->current_statement;
	BuildCommand* enclosing = con->current_build_command;
	BuildCommand* bc = build_command_inherit(&con->arena, con->current_build_command);
	con->current_build_command = bc;
	con->current_build_command->body = con->current_statement;

	for (size_t i = 0; i < e->argc; ++i) {
		SymbolValue arg = constructor_evaluate(con, e->args[i]);
		if (arg.type == SYMBOL_VALUE_STRING) {
			Target t = { .name = arg.string };
			da_append_arena(&con->arena, &bc->targets, t);
		}
	}


	con->current_build_command->body = outer;
	con->current_build_command = enclosing;

	return (SymbolValue){
		.type = SYMBOL_VALUE_BUILD_COMMAND,
		.bc = bc
	};
}

// NOTE: we have to wait for all the descriptions to end to run this,
// otherwise we might miss the compiler change
void constructor_expand_build_command_targets(Constructor* con, BuildCommand* bc) {
	for (size_t i = 0; i < bc->targets.count; ++i) {
		Target* t = &bc->targets.items[i];

		t->input_name.count = 0;
		if (bc->source_dir.count > 0) {
			da_append_many_arena(&con->arena, &t->input_name, bc->source_dir.items, bc->source_dir.count);
			da_append_arena(&con->arena, &t->input_name, '/');
		}
		da_append_many_arena(&con->arena, &t->input_name, t->name.items, t->name.count);
		if ((bc->compiler.count == 3 && strncmp(bc->compiler.items, "gcc", 3) == 0) ||
			(bc->compiler.count == 5 && strncmp(bc->compiler.items, "clang", 5) == 0)
		) {
			da_append_many_arena(&con->arena, &t->input_name, ".c", 2);
		} else if (bc->compiler.count == 3 && strncmp(bc->compiler.items, "g++", 3) == 0) {
			da_append_many_arena(&con->arena, &t->input_name, ".cpp", 4);
		}

		t->output_name.count = 0;

		if (bc->output_dir.count > 0) {
			da_append_many_arena(&con->arena, &t->output_name, bc->output_dir.items, bc->output_dir.count);
			da_append_arena(&con->arena, &t->output_name, '/');
		}
		da_append_many_arena(&con->arena, &t->output_name, t->name.items, t->name.count);
		if (bc->build_type == BUILD_OBJECT) {
			da_append_many_arena(&con->arena, &t->output_name, ".o", 2);
		}

		if (bc->parent) {
			da_append_arena(&con->arena, &bc->parent->input_files, sv_from_sb(t->output_name));
		}
	}

	for (size_t i = 0; i < bc->children.count; ++i) {
		constructor_expand_build_command_targets(con, bc->children.items[i]);
	}
}
