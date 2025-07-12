#include "interpreter.h"
#include "build_command.h"
#include "da.h"
#include "executer.h"
#include "expression.h"
#include "statement.h"
#include "symbol.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const SymbolValue nil = { .type = SYMBOL_VALUE_NIL };


Interpreter interpreter_new(BuildCommand* bc) {
	Interpreter in = {0};
	in.root_build_command = bc;
	in.current_environment = environment_new(&in.arena);
	return in;
}

void interpreter_interpret(Interpreter* in) {
	assert(in->root_build_command && in->root_build_command->body && "root bc body must not be null");

	interpreter_execute(in, in->root_build_command->body);
	execute_build_command(in->root_build_command);
}

void interpreter_error(Interpreter* in, Token token, const char* error_cstr) {
	in->had_error = true;
	fprintf(stderr,"[ERROR][interpreter] %zu:%zu %s\n\t%s %.*s\n",
		 token.line + 1, token.column,
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

BuildCommand* statement_find_attached_build_command(Statement* s, BuildCommand* bc) {
	if (!bc || !s || !bc->body) return NULL;
	if (bc->body == s)          return bc;
	for (size_t i = 0; i < bc->children.count; ++i) {
		BuildCommand* bcc = statement_find_attached_build_command(s, bc->children.items[i]);
		if (bcc) return bcc;
	}
	return NULL;
}

SymbolValue interpreter_execute(Interpreter* in, Statement* s) {
	BuildCommand* bc = statement_find_attached_build_command(s, in->root_build_command);
	if (bc && !bc->dirty && bc->parent != NULL) return nil;
	//if (bc != in->root_build_command) return nil;

	switch (s->type) {
		case STATEMENT_BLOCK:       interpret_block(in, &s->block); break;
		case STATEMENT_DESCRIPTION: interpret_description(in, &s->description); break;
		case STATEMENT_EXPRESSION:  interpreter_evaluate(in, s->expression.expression); break;
	}

	return nil;
}

SymbolValue interpret_block(Interpreter* in, StatementBlock*  s) {
	for (size_t i = 0; i < s->statement_count; ++i) {
		interpreter_execute(in, s->statements[i]);
	}
	return nil;
}

SymbolValue interpret_description(Interpreter* in, StatementDescription* s) {
	SymbolValue left = interpreter_execute(in, s->statement);

	for (size_t i = 0; i < s->block->block.statement_count; ++i) {
		interpreter_execute(in, s->block->block.statements[i]);
	}

	return left;
}

SymbolValue interpret_chain(Interpreter* in, ExpressionChain* e) {
	SymbolValue left = interpreter_evaluate(in, e->left);
	interpreter_evaluate(in, e->right);
	return left;
}

SymbolValue interpret_call(Interpreter* in, ExpressionCall* e) {
	SymbolValue callee = interpreter_evaluate(in, e->callee);
	Token callee_token = {
		.type = TOKEN_IDENTIFIER,
		.str.count = callee.string.count,
		.str.items = callee.string.items,
		.line = e->token.line,
		.column = e->token.column,
	};

	if (callee.type == SYMBOL_VALUE_NIL) {
		interpreter_error(in, callee_token, "callee is nil");
		return nil;
	}
	
	if (callee.type != SYMBOL_VALUE_METHOD) {
		interpreter_error(in, callee_token, "callee is not a method");
		return nil;
	}

	if (callee.method_type == METHOD_ECHO) {
		SymbolValue arg = interpreter_evaluate(in, e->args[0]);
		printf("%.*s\n",(int)arg.string.count, arg.string.items);
	}
	return nil;
}


SymbolValue interpreter_lookup_variable(Interpreter* in, StringView sv, Expression* e) {
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

