#include "statement.h"

StatementList statement_list_new(void) {
	StatementList statement_list = {
		.count = 0,
		.capacity = 0,
		.items = NULL,
	};
	return statement_list;
}

const char* statement_name_cstr(StatementType st) {
	switch (st) {
		#define CASE(T) case T: return #T;
		CASE(STATEMENT_EXPRESSION)
		CASE(STATEMENT_BLOCK)
		CASE(STATEMENT_DESCRIPTION)
		#undef CASE
	}
	return "!!! STATEMENT INVALID";
}

void statement_print(Statement* s, int indent) {
	if (!s) {
		return;
	}

	for (int i = 0; i < indent; ++i) {
		putchar(' ');
	}

	printf("%s\n", statement_name_cstr(s->type));

	switch (s->type) {
		case STATEMENT_EXPRESSION:
			expression_print(s->expression.expression, indent + 1);
			break;
		case STATEMENT_DESCRIPTION:
			statement_print(s->description.statement, indent + 2);
			statement_print(s->description.block, indent + 2);
			break;
		case STATEMENT_BLOCK:
			for (size_t i = 0; i < s->block.statement_count; ++i) {
				statement_print(s->block.statements[i], indent + 1);
			}
			break;
	}
}
