#include "expression.h"


const char* expression_name_cstr(ExpressionType et) {
	switch (et) {
		#define CASE(T) case T: return #T;
		CASE(EXPR_ASSIGNMENT)
		CASE(EXPR_LOGICAL)
		CASE(EXPR_BINARY)
		CASE(EXPR_UNARY)
		CASE(EXPR_CHAIN)
		CASE(EXPR_LITERAL_INT)
		CASE(EXPR_LITERAL_FLOAT)
		CASE(EXPR_LITERAL_STRING)
		CASE(EXPR_VARIABLE)
		CASE(EXPR_GROUPING)
		CASE(EXPR_CALL)
		#undef CASE
	}
	return "!!! EXPR INVALID";
}


inline static void print_indent(int indent) {
	for (int i = 0; i < indent; ++i) {
		putchar(' ');
	}
}
void expression_print(Expression* expr, int indent) {
	if (!expr) {
		return;
	}

	print_indent(indent);

	switch(expr->type) {
		case EXPR_VARIABLE:
			printf("variable: %.*s\n", (int)expr->variable.name.str.count, expr->variable.name.str.items);
			break;
		case EXPR_LITERAL_INT:
			printf("literal int: %d\n", expr->literal_int.value);
			break;
		case EXPR_LITERAL_STRING:
			printf("literal string: '%.*s'\n", (int)expr->literal_string.str.count, expr->literal_string.str.items);
			break;
		case EXPR_CALL:
			printf("call:\n");
			print_indent(indent + 2);
			printf("method:\n");
			expression_print(expr->call.callee, indent + 3);
			print_indent(indent + 2);
			printf("arguments:\n");
			for (size_t i = 0; i < expr->call.argc; ++i) {
				expression_print(expr->call.args[i], indent + 4);
			}
			break;
		case EXPR_CHAIN:
			printf("chain:\n");
			print_indent(indent + 2);
			printf("left:\n");
			expression_print(expr->chain.left, indent + 3);
			print_indent(indent + 2);
			printf("right:\n");
			expression_print(expr->chain.right, indent + 3);
			break;
		case EXPR_UNARY:
			printf("unary:\n");
			print_indent(indent + 2);
			printf("op: %.*s\n", (int)expr->unary.op.str.count, expr->unary.op.str.items);
			print_indent(indent + 2);
			printf("right:\n");
			expression_print(expr->unary.right, indent + 3);
			break;
		case EXPR_BINARY:
			printf("binary:\n");
			print_indent(indent + 2);
			printf("left:\n");
			expression_print(expr->binary.left, indent + 3);
			print_indent(indent + 2);
			printf("op: %.*s\n", (int)expr->binary.op.str.count, expr->binary.op.str.items);
			print_indent(indent + 2);
			printf("right:\n");
			expression_print(expr->binary.right, indent + 3);
			break;
		case EXPR_GROUPING:
			printf("grouping:\n");
			expression_print(expr->grouping.expr, indent + 3);
			break;
		default:
			printf("unknown expression type: %s\n", expression_name_cstr(expr->type));
			break;
	}
}
