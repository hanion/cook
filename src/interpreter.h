#pragma once
#include "da.h"
#include "expression.h"
#include "parser.h"
#include "symbol.h"
#include "build_command.h"


typedef struct Environment {
	struct Environment* enclosing;
	SymbolMap map;
	BuildCommand* current_build_command;
} Environment;

Environment environment_default();


typedef struct {
	Parser* parser;
	bool had_error;
	Arena arena;
	Environment root_env;
} Interpreter;

Interpreter interpreter_new(Parser*);

void interpreter_interpret(Interpreter*);
void interpreter_dry_run  (Interpreter*);

BuildCommand* interpreter_interpret_build_command(Interpreter*);

void interpreter_error(Interpreter* in, Token token, const char* error_cstr);

SymbolValue interpreter_evaluate(Interpreter* in, Expression* e);
void        interpreter_execute (Interpreter* in, Statement*  s);
void        interpret_block     (Interpreter* in, StatementBlock* s);
void        interpret_call      (Interpreter* in, ExpressionCall* e);

SymbolValue interpreter_lookup_variable(Interpreter* in, StringView sv, Expression* e);

