#pragma once
#include "symbol.h"
#include "build_command.h"

typedef struct {
	Arena arena;
	bool had_error;
	Environment*  current_environment;
	BuildCommand* current_build_command;
	Statement*    current_statement;
} Constructor;

Constructor constructor_new(Statement*);

BuildCommand* constructor_construct_build_command(Constructor*);

void constructor_analyze(Constructor*, BuildCommand*);


void constructor_error(Constructor* con, Token token, const char* error_cstr);

SymbolValue constructor_evaluate(Constructor* con, Expression* e);
SymbolValue constructor_execute (Constructor* con, Statement*  s);

SymbolValue constructor_lookup_variable(Constructor* con, StringView sv, Expression* e);

SymbolValue constructor_interpret_block       (Constructor* con, StatementBlock*  s);
SymbolValue constructor_interpret_call        (Constructor* con, ExpressionCall*  e);
SymbolValue constructor_interpret_chain       (Constructor* con, ExpressionChain* e);
SymbolValue constructor_interpret_description (Constructor* con, StatementDescription* s);
SymbolValue constructor_interpret_method_build(Constructor* con, ExpressionCall* e);

void constructor_expand_build_command_targets(Constructor* con, BuildCommand* bc);
