#pragma once
#include "da.h"
#include "expression.h"
#include "statement.h"
#include "symbol.h"
#include "build_command.h"


typedef struct {
	Arena arena;
	int verbose;
	bool had_error;
	BuildCommand* root_build_command;
	Environment* current_environment;
} Interpreter;

Interpreter interpreter_new(BuildCommand*);
void interpreter_interpret (Interpreter*);

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
