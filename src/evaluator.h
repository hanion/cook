#pragma once
#include "expression.h"
#include "symbol.h"

typedef struct {
	bool had_error;
	Environment** environment;
} Evaluator;

Evaluator evaluator_new(Environment** env);

SymbolValue evaluator_evaluate(Evaluator* ev, Expression* e);

SymbolValue evaluator_lookup_variable(Evaluator* ev, Expression* e);
