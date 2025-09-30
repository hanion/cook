#include "evaluator.h"

static const SymbolValue nill = { .type = SYMBOL_VALUE_NIL };

Evaluator evaluator_new(Environment** env) {
	return (Evaluator){
		.environment = env
	};
}


SymbolValue evaluator_evaluate(Evaluator* ev, Expression* e) {
	if (!e) { return nill; }

	switch (e->type) {
		case EXPR_VARIABLE: return evaluator_lookup_variable(ev, e);
		//case EXPR_CALL: ...
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


SymbolValue evaluator_lookup_variable(Evaluator* ev, Expression* e) {
	// TODO: symbol map in ev, check the variable if exists return value
	StringView sv = e->variable.name.str;
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
