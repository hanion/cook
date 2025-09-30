#include "executor.h"
#include "build_command.h"
#include "da.h"
#include "evaluator.h"
#include "expression.h"
#include "statement.h"
#include "symbol.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const SymbolValue nil = { .type = SYMBOL_VALUE_NIL };

SymbolValue execute_call       (Executor* ex, ExpressionCall* e);
SymbolValue execute_chain      (Executor* ex, ExpressionChain* e);
SymbolValue executor_execute   (Executor* ex, Statement* s);
SymbolValue execute_description(Executor* ex, StatementDescription* s);
SymbolValue execute_block      (Executor* ex, StatementBlock* s);





Executor executor_new(BuildCommand* bc, bool dry_run) {
	Executor ex = {};
	ex.root_build_command = bc;
	ex.dry_run = dry_run;
	ex.current_environment = environment_new(&ex.arena);
	ex.evaluator = evaluator_new(&ex.current_environment);
	ex.runner = runner_new(&ex.arena);
	return ex;
}


void executor_run(Executor* ex) {
	assert(ex->root_build_command && ex->root_build_command->body && "root bc body must not be null");
	executor_execute(ex, ex->root_build_command->body);
}




void executeer_error(Executor* ex, Token token, const char* error_cstr) {
	ex->had_error = true;
	fprintf(stderr,"[ERROR][Executor] %zu:%zu %s\n\t%s %.*s\n",
		 token.line + 1, token.column,
		 error_cstr, token_name_cstr(token), (int)token.str.count, token.str.items);
	raise(1);
}



SymbolValue executor_evaluate(Executor* ex, Expression* e) {
	if (!e) { return (SymbolValue){0}; }

	switch (e->type) {
		case EXPR_LITERAL_STRING:
		case EXPR_VARIABLE: return evaluator_evaluate(&ex->evaluator, e);
		case EXPR_CALL:     return execute_call(ex, &e->call);
		case EXPR_CHAIN:    return execute_chain(ex, &e->chain);
		default: break;
	}
	return (SymbolValue){0};
}

BuildCommand* statement_find_attached_build_command(Statement* s, BuildCommand* bc) {
	if (!bc || !s || !bc->body) return NULL;

	if (bc->body == s) return bc;

	for (size_t i = 0; i < bc->children.count; ++i) {
		BuildCommand* bcc = statement_find_attached_build_command(s, bc->children.items[i]);
		if (bcc) return bcc;
	}

	return NULL;
}

SymbolValue executor_execute(Executor* ex, Statement* s) {
	BuildCommand* bc = statement_find_attached_build_command(s, ex->root_build_command);
	if (bc && !bc->dirty && bc->parent != NULL) return nil;

	switch (s->type) {
		case STATEMENT_EXPRESSION:  executor_evaluate(ex, s->expression.expression); break;
		case STATEMENT_BLOCK:       execute_block(ex, &s->block); break;
		case STATEMENT_DESCRIPTION: execute_description(ex, &s->description); break;
	}
	
	if (bc) {
		if (ex->dry_run) {
			runner_dry_run(&ex->runner, bc);
		} else {
			runner_execute(&ex->runner, bc);
		}
	}

	return nil;
}

SymbolValue execute_block(Executor* ex, StatementBlock*  s) {
	for (size_t i = 0; i < s->statement_count; ++i) {
		executor_execute(ex, s->statements[i]);
	}
	return nil;
}

SymbolValue execute_description(Executor* ex, StatementDescription* s) {
	SymbolValue left = executor_execute(ex, s->statement);

	for (size_t i = 0; i < s->block->block.statement_count; ++i) {
		executor_execute(ex, s->block->block.statements[i]);
	}

	return left;
}

SymbolValue execute_chain(Executor* ex, ExpressionChain* e) {
	SymbolValue left = executor_evaluate(ex, e->left);
	executor_evaluate(ex, e->right);
	return left;
}

SymbolValue execute_call(Executor* ex, ExpressionCall* e) {
	SymbolValue callee = executor_evaluate(ex, e->callee);
	Token callee_token = {
		.type = TOKEN_IDENTIFIER,
		.str.count = callee.string.count,
		.str.items = callee.string.items,
		.line = e->token.line,
		.column = e->token.column,
	};

	if (callee.type == SYMBOL_VALUE_NIL) {
		executeer_error(ex, callee_token, "callee is nil");
		return nil;
	}
	
	if (callee.type != SYMBOL_VALUE_METHOD) {
		executeer_error(ex, callee_token, "callee is not a method");
		return nil;
	}

	if (callee.method_type == METHOD_ECHO) {
		SymbolValue arg = executor_evaluate(ex, e->args[0]);
		printf("%.*s\n",(int)arg.string.count, arg.string.items);
	} else {
		/* not sure
		Expression expr = {
			.type = EXPR_CALL,
			.call = *e
		};
		return evaluator_evaluate(&ex->evaluator, &expr);
		*/
	}
	return nil;
}



