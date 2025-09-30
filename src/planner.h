#pragma once
#include "evaluator.h"
#include "symbol.h"
#include "build_command.h"

typedef struct {
	Arena arena;
	bool had_error;
	Environment*  current_environment;
	BuildCommand* current_build_command;
	Statement*    current_statement;
	Evaluator     evaluator;
} Planner;
// TODO: keep track of the current Cookfile, for better error messages

Planner planner_new(Statement*);

BuildCommand* planner_construct_build_command(Planner*);

void planner_analyze(Planner*, BuildCommand*);


void planner_error(Planner* con, Token token, const char* error_cstr);

SymbolValue planner_evaluate(Planner* con, Expression* e);
SymbolValue planner_execute (Planner* con, Statement*  s);

SymbolValue planner_lookup_variable(Planner* con, StringView sv, Expression* e);

SymbolValue planner_interpret_block       (Planner* con, StatementBlock*  s);
SymbolValue planner_interpret_call        (Planner* con, ExpressionCall*  e);
SymbolValue planner_interpret_chain       (Planner* con, ExpressionChain* e);
SymbolValue planner_interpret_description (Planner* con, StatementDescription* s);
SymbolValue planner_interpret_method_build(Planner* con, ExpressionCall* e);

void planner_expand_build_command_targets(Planner* con, BuildCommand* bc);
