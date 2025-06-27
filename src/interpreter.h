#pragma once
#include "da.h"
#include "expression.h"
#include "parser.h"
#include "statement.h"
#include "symbol.h"
#include "build_command.h"


typedef struct Environment {
	struct Environment* enclosing;
	SymbolMap map;
} Environment;


typedef struct {
	Parser* parser;
	bool had_error;
	Arena arena;
	Environment* current_environment;
	BuildCommand* current_build_command;
	int verbose;
} Interpreter;

Environment* environment_new(Arena*);
Interpreter interpreter_new(Parser*);

void interpreter_interpret(Interpreter*);
void interpreter_dry_run  (Interpreter*);

BuildCommand* interpreter_interpret_build_command(Interpreter*);

void interpreter_error(Interpreter* in, Token token, const char* error_cstr);

SymbolValue interpreter_evaluate (Interpreter* in, Expression* e);
SymbolValue interpreter_execute  (Interpreter* in, Statement*  s);
SymbolValue interpret_block      (Interpreter* in, StatementBlock*  s);
SymbolValue interpret_call       (Interpreter* in, ExpressionCall*  e);
SymbolValue interpret_chain      (Interpreter* in, ExpressionChain* e);
SymbolValue interpret_description(Interpreter* in, StatementDescription* s);

SymbolValue interpreter_lookup_variable(Interpreter* in, StringView sv, Expression* e);


SymbolValue interpret_method_build(Interpreter* in, ExpressionCall* e);

void interpreter_expand_build_command_targets(Interpreter* in, BuildCommand* bc);
